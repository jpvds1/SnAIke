import numpy as np
import random
import json
import os
from agents.base_agent import Agent
from core.snake import Direction, _is_opposite

_SCHEMA_VERSION = 1.0
 
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
        pop_size: int = 50,
        elite_count: int = 5,
        mutation_rate: float = 0.15,
        mutation_strength: float = 0.2,
        tournament_size: int = 5
    ):
        self.trainable = True
        self.pop_size = pop_size
        self.layer_sizes = [9, 15, 10, 4]

        self.population = [NeuralNetwork(self.layer_sizes) for _ in range(pop_size)]
        self.fitness_scores = [0.0] * pop_size
        self.scores = [0] * pop_size
        self.current_idx = 0
        self.generation = 1

        self.episodes_per_generation = pop_size

        self.elite_count = elite_count
        self.mutation_rate = mutation_rate
        self.mutation_strength = mutation_strength
        self.tournament_size = tournament_size

        self.load()

    # ---------------------------------------------------------------------------
    # API
    # ---------------------------------------------------------------------------

 
    def get_action(self, observation: np.ndarray) -> Direction | None:
        nn = self.population[self.current_idx]
        output = nn.forward(observation)

        # Find current direction and filter it
        curr_dir_idx = np.argmax(observation[3:7])
        directions = [Direction.UP, Direction.DOWN, Direction.LEFT, Direction.RIGHT]
        curr_dir = directions[curr_dir_idx]
        for i, move in enumerate(directions):
            if _is_opposite(move, curr_dir):
                output[i] = -np.inf

        action_idx = np.argmax(output)
        return directions[action_idx]

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

            child_weights = self._crossover(p1, p2)
            child_weights = self._mutate(child_weights)

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
        noise = np.random.randn(len(weights)) * self.mutation_strength
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