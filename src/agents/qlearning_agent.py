import numpy as np
from agents.base_agent import Agent
from core.snake import Direction
 
 
class QLearningAgent(Agent):
    trainable = True
    display_name = "Q-Learning"
    description = "Learns a state-action value table through reward-driven exploration."
 
    def get_action(self, observation: np.ndarray) -> Direction | None:
        raise NotImplementedError
 
    @classmethod
    def load_stats(cls) -> dict:
        return {
            "highest_score": 0,
            "current_generation": 0,
            "avg_score": 0.0,
        }