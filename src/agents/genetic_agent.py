import numpy as np
import random
import json
import os
from agents.base_agent import Agent
from core.snake import Direction, _is_opposite

_SCHEMA_VERSION = 2.0

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _dir_to_vec(direction: Direction) -> tuple[int, int]:
    if direction == Direction.UP:    return ( 0,-1)
    if direction == Direction.DOWN:  return ( 0, 1)
    if direction == Direction.LEFT:  return (-1, 0)
    if direction == Direction.RIGHT: return ( 1, 0)

def _vec_to_dir(dx: int, dy: int) -> Direction:
    return {
        ( 0,-1): Direction.UP,
        ( 0, 1): Direction.DOWN,
        (-1, 0): Direction.LEFT,
        ( 1, 0): Direction.RIGHT
    }[(dx, dy)]

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
        head            = state["head"]
        direction       = state["direction"]
        apple           = state["apple"]
        score           = state["score"]
        steps           = state["steps"]
        hunger          = state["hunger"]
        board_size      = state["board_size"]

        hx, hy = head
        ax, ay = apple
        bw, bh = board_size

        snake_size = float(len(snake_positions)) / (bw * bh) 

        body_set = {tuple(p) for p in snake_positions[1:]}

        dx, dy = _dir_to_vec(direction)
        rdx, rdy = _rotate_cw(dx, dy)
        ldx, ldy = _rotate_ccw(dx, dy)

        body_ahead = cls._body_dist(hx, hy,  dx,  dy, body_set, bw, bh)
        body_left  = cls._body_dist(hx, hy, ldx, ldy, body_set, bw, bh)
        body_right = cls._body_dist(hx, hy, rdx, rdy, body_set, bw, bh)

        wall_ahead  = cls._wall_dist(hx, hy,  dx,  dy, bw, bh)
        wall_behind = cls._wall_dist(hx, hy, -dx, -dy, bw, bh)
        wall_left   = cls._wall_dist(hx, hy, ldx, ldy, bw, bh)
        wall_right  = cls._wall_dist(hx, hy, rdx, rdy, bw, bh)

        direction_x = float(dx)
        direction_y = float(dy)

        rel_x = ax - hx
        rel_y = ay - hy

        apple_fwd  = rel_x * dx + rel_y * dy
        apple_side = rel_x * rdx + rel_y * rdy

        apple_ahead_val = (1.0 / apple_fwd) if apple_fwd != 0 else 0.0
        apple_side_val = (1.0 / apple_side) if apple_side != 0 else 0.0

        return np.array([
            body_ahead, body_right, body_left,
            wall_ahead, wall_behind, wall_right, wall_left,
            direction_x, direction_y,
            apple_ahead_val, apple_side_val,
            snake_size
        ], dtype=np.float64)

    @staticmethod
    def _body_dist(hx: int, hy: int, dx: int, dy: int, body_set: set, bw: int, bh: int) -> float:
        steps = 1
        while True:
            nx, ny = hx + dx * steps, hy + dy * steps
            if not (0 <= nx < bw and 0 <= ny < bh):
                return 0.0
            if (nx, ny) in body_set:
                return 1.0 / steps
            steps += 1

    @staticmethod
    def _wall_dist(hx: int, hy: int, dx: int, dy: int, bw: int, bh: int) -> float:
        steps_x = (bw - 1 - hx) if dx > 0 else (hx if dx < 0 else float("inf"))
        steps_y = (bh - 1 - hy) if dy > 0 else (hy if dy < 0 else float("inf"))
        steps = min(steps_x, steps_y)
        return 1.0 / steps if steps > 0 else 1.0

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


class GeneticAgent(Agent):
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
        tournament_size: int = 5
    ):
        self.trainable = True
        self.pop_size = pop_size
        self.layer_sizes = [12, 16, 3]

        self.population = [NeuralNetwork(self.layer_sizes) for _ in range(pop_size)]
        self.fitness_scores = [0.0] * pop_size
        self.scores = [0] * pop_size
        self.current_idx = 0
        self.generation = 1

        self.episodes_per_generation = pop_size

        self.elite_count = elite_count
        self.mutation_rate = mutation_rate
        self.mutation_strength = mutation_strength
        self.min_mutation_strength = min_mutation_strength
        self.mut_stren_dropoff = mut_stren_dropoff
        self.tournament_size = tournament_size

        self.load()

    # ---------------------------------------------------------------------------
    # API
    # ---------------------------------------------------------------------------

 
    def get_action(self, state: dict) -> Direction | None:
        obs    = ObservationWrapper.observation_from_state(state)
        nn     = self.population[self.current_idx]
        output = nn.forward(obs)

        action_idx = int(np.argmax(output))

        dx, dy = _dir_to_vec(state["direction"])
        if action_idx == 0:
            new_dx, new_dy = _rotate_ccw(dx, dy)
        elif action_idx == 1:
            new_dx, new_dy = _rotate_cw(dx, dy)
        else:
            new_dx, new_dy = dx, dy

        return _vec_to_dir(new_dx, new_dy)

    def on_episode_end(self, stats: dict):
        score = stats.get("score", 0)
        steps = stats.get("steps", 0)

        fitness = (score * 5000) - (steps * 2) + 1000
        self.fitness_scores[self.current_idx] = fitness
        self.scores[self.current_idx] = score
        self.current_idx += 1


    def on_generation_end(self, gen_stats):
        print(f"[GeneticAlgorithm] Gen {self.generation} | Best Score: {max(self.scores)} | Avg Score: {sum(self.scores) / len(self.scores)} | Best Fitness: {max(self.fitness_scores):.1f} | Avg Fitness: {sum(self.fitness_scores) / len(self.fitness_scores)}")

        ranked = sorted(zip(self.fitness_scores, self.population), key=lambda x: x[0], reverse=True)

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
        self.fitness_scores = [0.0] * self.pop_size
        self.scores = [0] * self.pop_size
        self.current_idx = 0
        self.generation += 1

        self.save()

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
    # Persistence
    # ---------------------------------------------------------------------------

    def save(self):
        state = {
            "agent": "GeneticAlgorithm",
            "version": _SCHEMA_VERSION,
            "generation": self.generation,
            "hyperparameters": {
                "pop_size":          self.pop_size,
                "elite_count":       self.elite_count,
                "mutation_rate":     self.mutation_rate,
                "mutation_strength": self.mutation_strength,
                "tournament_size":   self.tournament_size,
                "layer_sizes":       self.layer_sizes
            },
            "population_weights": [nn.get_flat().tolist() for nn in self.population]
        }
        os.makedirs("./checkpoints", exist_ok=True)
        file_path = os.path.join("./checkpoints", f"{"GeneticAlgorithm"}.json")
        with open(file_path, "w") as f:
            json.dump(state, f)
        #print(f"[GeneticAlgorithm] Generation {self.generation} saved to {file_path}")

    def load(self):
        file_path = os.path.join("./checkpoints", f"{"GeneticAlgorithm"}.json")
        if not os.path.exists(file_path):
            return

        try:
            with open(file_path, "r") as f:
                state = json.load(f)

            agent = state.get("agent", "GeneticAlgorithm")
            if agent != "GeneticAlgorithm":
                raise "[Load] Agent missmatch"
            version = state.get("version", _SCHEMA_VERSION)
            if version != _SCHEMA_VERSION:
                raise "[Load] Schema version missmatch"

            self.generation =        state.get("generation", 1)
            hyperparameters =        state.get("hyperparameters", {})
            self.pop_size =          hyperparameters.get("pop_size", 50)
            self.elite_count =       hyperparameters.get("elite_count", 5)
            self.mutation_rate =     hyperparameters.get("mutation_rate", 0.15)
            self.mutation_strength = hyperparameters.get("mutation_strength", 0.2)
            self.tournament_size =   hyperparameters.get("tournament_size", 5)
            self.layer_sizes =       hyperparameters.get("layer_sizes", [9, 15, 10, 4])

            weights_list = state.get("population_weights", [])
            for i, weights in enumerate(weights_list):
                if i < len(self.population):
                    self.population[i].set_flat(np.array(weights))

        except Exception as e:
            print(f"[GeneticAlgorithm] Failed to load state: {e}")