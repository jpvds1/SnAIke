import numpy as np
import random
from agents.base_agent import Agent
from core.snake import Direction
 
 
class RandomAgent(Agent):
    trainable = True
    display_name = "Random"
    description = "Chooses a direction at random every step."
 
    def get_action(self, observation: np.ndarray) -> Direction | None:
        return random.choice(list(Direction))
 
    @classmethod
    def load_stats(cls) -> dict:
        return {
            "highest_score": 0,
            "current_generation": 0,
            "avg_score": 0.0,
        }