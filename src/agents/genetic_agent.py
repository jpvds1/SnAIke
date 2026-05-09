import numpy as np
from agents.base_agent import Agent
from core.snake import Direction
 
 
class GeneticAgent(Agent):
    trainable = True
    display_name = "Genetic Algorithm"
    description = "Evolves a fixed-topology network by selecting and mutating the fittest agents."
 
    def get_action(self, observation: np.ndarray) -> Direction | None:
        raise NotImplementedError
 
    @classmethod
    def load_stats(cls) -> dict:
        return {
            "highest_score": 0,
            "current_generation": 0,
            "avg_score": 0.0,
        }