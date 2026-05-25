import numpy as np
import random
import json
import os
import numba
from agents.base_agent import Agent, Genome, PopulationAgent
from core.snake import Direction, _is_opposite

_SCHEMA_VERSION = 3.0

# ---------------------------------------------------------------------------
# Numba
# ---------------------------------------------------------------------------

@numba.njit(cache=True)
def _nb_ray(hx: int, hy: int, dx: int, dy: int, occupied: np.ndarray, bw: int, bh: int) -> float:
    s = 1
    while True:
        nx = hx + dx * s
        ny = hy + dy * s
        if nx < 0 or nx >= bw or ny < 0 or ny >= bh:
            return 0.0
        if occupied[nx, ny]:
            return 1.0 / s
        s += 1

@numba.njit(cache=True)
def _nb_wall(hx: int, hy: int, dx: int, dy: int, bw: int, bh: int) -> float:
    _INF = bw * bh + 1
    sx = (bw - 1 - hx) if dx > 0 else (hx if dx < 0 else _INF)
    sy = (bh - 1 - hy) if dy > 0 else (hy if dy < 0 else _INF)
    s = sx if sx < sy else sy
    return 1.0 / s if s > 0 else 1.0

@numba.njit(cache=True)
def _nb_compute_obs(
    hx: int, hy: int,
    dx: int, dy: int,
    rdx: int, rdy: int,
    ldx: int, ldy: int,
    ax: int, ay: int,
    bw: int, bh: int,
    snake_size: float,
    body: np.ndarray) -> np.ndarray:
    occupied = np.zeros((bw, bh), dtype=np.bool_)
    for i in range(body.shape[0]):
        occupied[body[i, 0], body[i, 1]] = True

    ba = _nb_ray(hx, hy,  dx,  dy, occupied, bw, bh)
    bl = _nb_ray(hx, hy, ldx, ldy, occupied, bw, bh)
    br = _nb_ray(hx, hy, rdx, rdy, occupied, bw, bh)

    wa = _nb_wall(hx, hy,  dx,  dy, bw, bh)
    wb = _nb_wall(hx, hy, -dx, -dy, bw, bh)
    wl = _nb_wall(hx, hy, ldx, ldy, bw, bh)
    wr = _nb_wall(hx, hy, rdx, rdy, bw, bh)

    rel_x = float(ax - hx)
    rel_y = float(ay - hy)
    afwd  = rel_x * dx + rel_y * dy
    aside = rel_x * rdx + rel_y * rdy
    av    = 1.0 / afwd if afwd != 0.0 else 0.0
    asv   = 1.0 / aside if aside != 0.0 else 0.0

    return np.array(
        [ba, br, bl, wa, wb, wr, wl, float(dx), float(dy), av, asv, snake_size],
        dtype=np.float64,
    )

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

_DIR_TO_VEC: dict[Direction, tuple[int, int]] = {
    Direction.UP:    ( 0, -1),
    Direction.DOWN:  ( 0,  1),
    Direction.LEFT:  (-1,  0),
    Direction.RIGHT: ( 1,  0),
}

_VEC_TO_DIR: dict[tuple[int, int], Direction] = {v: k for k, v in _DIR_TO_VEC.items()}

def _rotate_cw(dx: int, dy: int) -> tuple[int, int]:
    return (-dy, dx)

def _rotate_ccw(dx: int, dy: int) -> tuple[int, int]:
    return (dy, -dx)

# ---------------------------------------------------------------------------
# Observation Wrapper
# ---------------------------------------------------------------------------
 
class ObservationWrapper:
    @classmethod
    def observation_from_state(cls, state: dict) -> np.ndarray:
        snake_positions = state["snake_positions"]
        hx, hy          = state["head"]
        ax, ay          = state["apple"]
        bw, bh          = state["board_size"]

        snake_size = float(len(snake_positions)) / (bw * bh) 

        dx, dy   = _DIR_TO_VEC[state["direction"]]
        rdx, rdy = _rotate_cw(dx, dy)
        ldx, ldy = _rotate_ccw(dx, dy)

        body_list = snake_positions[1:]
        body = (
            np.array(body_list, dtype=np.int64).reshape(-1, 2)
            if body_list
            else np.empty((0, 2), dtype=np.int64)
        )
        return _nb_compute_obs(
            hx, hy, dx, dy, rdx, rdy, ldx, ldy,
            ax, ay, bw, bh, snake_size, body,
        )

# ---------------------------------------------------------------------------
# Neural Network
# ---------------------------------------------------------------------------

class NeuralNetwork:
    def __init__(self, layer_sizes: list[int]):
        self.layer_sizes = layer_sizes
        self.weights = []
        self.biases = []

        for i in range(len(self.layer_sizes) - 1):
            w = np.random.randn(layer_sizes[i], layer_sizes[i + 1]) * np.sqrt(2.0 / layer_sizes[i])
            b = np.zeros(layer_sizes[i + 1])
            self.weights.append(w)
            self.biases.append(b)

    def forward(self, x: np.ndarray) -> np.ndarray:
        for i, (w, b) in enumerate(zip(self.weights, self.biases)):
            x = x @ w + b
            if i < len(self.weights) - 1:
                x = np.tanh(x)
        return x

    def get_flat(self) -> np.ndarray:
        parts = [w.flatten() for w in self.weights] + [b.flatten() for b in self.biases]
        return np.concatenate(parts)

    def set_flat(self, flat: np.ndarray):
        idx = 0
        for w in self.weights:
            size = w.size
            w[:] = flat[idx:idx + size].reshape(w.shape)
            idx += size
        for b in self.biases:
            size = b.size
            b[:] = flat[idx:idx + size]
            idx += size

    def clone(self) -> "NeuralNetwork":
        child = NeuralNetwork(self.layer_sizes)
        child.set_flat(self.get_flat().copy())
        return child

    def to_dict(self) -> dict:
        return {
            "layer_sizes": self.layer_sizes,
            "weights": [w.tolist() for w in self.weights],
            "biases": [b.tolist() for b in self.biases]
        }

    @classmethod
    def from_dict(cls, data: dict) -> "NeuralNetwork":
        nn = cls(data["layer_sizes"])
        nn.weights = [np.array(w, dtype=np.float64) for w in data["weights"]]
        nn.biases = [np.array(b, dtype=np.float64) for b in data["biases"]]
        return nn

# ---------------------------------------------------------------------------
# Shared helper
# ---------------------------------------------------------------------------

def _nn_act(nn: NeuralNetwork, state: dict) -> Direction:
    obs = ObservationWrapper.observation_from_state(state)
    output = nn.forward(obs)
    action_idx = int(np.argmax(output))

    dx, dy = _DIR_TO_VEC[state["direction"]]
    if action_idx == 0:
        new_dx, new_dy = _rotate_ccw(dx, dy)
    elif action_idx == 1:
        new_dx, new_dy = _rotate_cw(dx, dy)
    else:
        new_dx, new_dy = dx, dy

    return _VEC_TO_DIR[(new_dx, new_dy)]

# ---------------------------------------------------------------------------
# GeneticGenome
# ---------------------------------------------------------------------------

class GeneticGenome(Genome):
    def __init__(self, nn: NeuralNetwork) -> None:
        self._nn = nn

    def get_action(self, state: dict) -> Direction:
        return _nn_act(self._nn, state)

# ---------------------------------------------------------------------------
# Checkpoint filenames
# ---------------------------------------------------------------------------

_CKPT_DIR     = "./checkpoints"
_META_FILE    = "GeneticAlgorithm_meta.json"
_WEIGHTS_FILE = "GeneticAlgorithm_weights"

# ---------------------------------------------------------------------------
# Genetic Agent
# ---------------------------------------------------------------------------

class GeneticAgent(PopulationAgent):
    display_name = "Genetic Algorithm"
    description = "Evolves a fixed-topology network by selecting and mutating the fittest agents."

    def __init__(
        self, 
        pop_size: int = 200,
        elite_count: int = 5,
        mutation_rate: float = 0.10,
        mutation_strength: float = 0.2,
        min_mutation_strength: float = 0.05,
        mut_stren_dropoff: int = 40,
        tournament_size: int = 5,
        save_interval: int = 20
    ):
        self.trainable = True
        self.pop_size = pop_size
        self.layer_sizes = [12, 16, 3]

        self.population = [NeuralNetwork(self.layer_sizes) for _ in range(pop_size)]
        self.generation = 1

        self.elite_count = elite_count
        self.mutation_rate = mutation_rate
        self.mutation_strength = mutation_strength
        self.min_mutation_strength = min_mutation_strength
        self.mut_stren_dropoff = mut_stren_dropoff
        self.tournament_size = tournament_size
        self.save_interval = save_interval

        self._best_score_ever: int = 0
        self._play_nn: NeuralNetwork = self.population[0]

        self.load()

    # ---------------------------------------------------------------------------
    # PopulationAgent
    # ---------------------------------------------------------------------------

    def get_population(self) -> list[GeneticGenome]:
        return [GeneticGenome(nn) for nn in self.population]

    def evolve(self, results: list[dict]) -> None:
        fitness_scores = [self._compute_fitness(r) for r in results]
        scores         = [r["score"] for r in results]

        gen_best = max(scores)
        is_new_best = gen_best > self._best_score_ever
        if is_new_best:
            self._best_score_ever = gen_best

        self._log_generation(scores, fitness_scores)

        ranked = sorted(
            zip(fitness_scores, self.population),
            key=lambda x: x[0],
            reverse=True,
        )

        new_population: list[NeuralNetwork] = []

        for _, nn in ranked[:self.elite_count]:
            new_population.append(nn.clone())

        while len(new_population) < self.pop_size:
            p1 = self._tournament_select(ranked)
            p2 = self._tournament_select(ranked)

            child_weights = self._mutate(self._crossover(p1, p2))
            child = NeuralNetwork(self.layer_sizes)
            child.set_flat(child_weights)
            new_population.append(child)
 
        self.population = new_population
        self.generation += 1
 
        self.save(force=is_new_best)

    # ---------------------------------------------------------------------------
    # BaseAgent
    # ---------------------------------------------------------------------------
 
    def get_action(self, state: dict) -> Direction:
        return _nn_act(self._play_nn, state)

    @classmethod
    def load_stats(cls) -> dict:
        return {
            "highest_score": 0,
            "current_generation": 0,
            "avg_score": 0.0,
        }

    # ---------------------------------------------------------------------------
    # Evolution
    # ---------------------------------------------------------------------------

    def _compute_fitness(self, result: dict) -> float:
        score = result.get("score", 0)
        steps = result.get("steps", 0)
        fitness = steps + score ** 4 - ((steps **  1.5) / (score + 1))
        return max(0.1, fitness)

    def _tournament_select(self, ranked: list) -> NeuralNetwork:
        contestants = random.sample(ranked, min(self.tournament_size, len(ranked)))
        return max(contestants, key=lambda x: x[0])[1]

    def _crossover(self, p1: NeuralNetwork, p2: NeuralNetwork) -> np.ndarray:
        w1, w2 = p1.get_flat(), p2.get_flat()
        mask = np.random.rand(len(w1)) < 0.5
        return np.where(mask, w1, w2)

    def _mutate(self, weights: np.ndarray) -> np.ndarray:
        mutation_mask = np.random.rand(len(weights)) < self.mutation_rate
        mut_stren = self.mutation_strength - 0.01 * (self.generation // self.mut_stren_dropoff)
        mut_stren = max(mut_stren, self.min_mutation_strength)
        noise = np.random.randn(len(weights)) * mut_stren
        weights[mutation_mask] += noise[mutation_mask]
        return weights

    # ---------------------------------------------------------------------------
    # Logging
    # ---------------------------------------------------------------------------

    def _log_generation(self, scores: list[int], fitness_scores: list[float]) -> None:
        print(
            f"[GeneticAgent] Gen {self.generation} | "
            f"Best Score: {max(scores)} | "
            f"Avg Score: {sum(scores) / len(scores):.2f} | "
            f"Best Fitness: {max(fitness_scores):.1f} | "
            f"Avg Fitness: {sum(fitness_scores) / len(fitness_scores):.1f}"
        )

    # ---------------------------------------------------------------------------
    # Persistence
    # ---------------------------------------------------------------------------

    def save(self, force: bool = False):
        if not force and (self.generation % self.save_interval != 0):
            return

        os.makedirs(_CKPT_DIR, exist_ok=True)

        state = {
            "agent": "GeneticAlgorithm",
            "version": _SCHEMA_VERSION,
            "generation": self.generation,
            "best_score_ever": self._best_score_ever,
            "hyperparameters": {
                "pop_size":              self.pop_size,
                "elite_count":           self.elite_count,
                "mutation_rate":         self.mutation_rate,
                "mutation_strength":     self.mutation_strength,
                "min_mutation_strength": self.min_mutation_strength,
                "mut_stren_dropoff":     self.mut_stren_dropoff,
                "tournament_size":       self.tournament_size,
                "layer_sizes":           self.layer_sizes,
                "save_interval":         self.save_interval
            },
        }
        
        file_path = os.path.join(_CKPT_DIR, _META_FILE)
        with open(file_path, "w") as f:
            json.dump(state, f, indent=2)

        weights_matrix = np.array(
            [nn.get_flat() for nn in self.population], dtype=np.float64
        )
        np.savez_compressed(os.path.join(_CKPT_DIR, _WEIGHTS_FILE), weights=weights_matrix)        

    def load(self):
        meta_path    = os.path.join(_CKPT_DIR, _META_FILE)
        weights_path = os.path.join(_CKPT_DIR, _WEIGHTS_FILE + ".npz")

        if not os.path.exists(meta_path):
            print("[GeneticAlgorithm] No available save file found.")
            return            

        with open(meta_path, "r") as f:
            meta = json.load(f)

        if meta.get("agent") != "GeneticAlgorithm":
            raise ValueError("Agent mismatch in checkpoint metadata.")
        if meta.get("version") != _SCHEMA_VERSION:
            raise ValueError(
                f"Schema version mismatch: expected {_SCHEMA_VERSION}, "
                f"got {meta.get('version')}."
            )

        self.generation        = meta.get("generation", 1)
        self._best_score_ever  = meta.get("best_score_ever", 0)

        hp = meta.get("hyperparameters", {})
        self.pop_size             = hp.get("pop_size",              self.pop_size)
        self.elite_count          = hp.get("elite_count",           self.elite_count)
        self.mutation_rate        = hp.get("mutation_rate",         self.mutation_rate)
        self.mutation_strength    = hp.get("mutation_strength",     self.mutation_strength)
        self.min_mutation_strength= hp.get("min_mutation_strength", self.min_mutation_strength)
        self.mut_stren_dropoff    = hp.get("mut_stren_dropoff",     self.mut_stren_dropoff)
        self.tournament_size      = hp.get("tournament_size",       self.tournament_size)
        self.layer_sizes          = hp.get("layer_sizes",           self.layer_sizes)
        self.save_interval        = hp.get("save_interval",         self.save_interval)

        while len(self.population) < self.pop_size:
            self.population.append(NeuralNetwork(self.layer_sizes))
        self.population = self.population[:self.pop_size]

        npz = np.load(weights_path)
        for i, flat in enumerate(npz["weights"]):
            if i < len(self.population):
                self.population[i].set_flat(flat)

        self._play_nn = self.population[0]