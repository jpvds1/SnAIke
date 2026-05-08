import numpy as np
from agents.base_agent import Agent
from ui.input_handler import InputHandler
from core.snake import Direction

class HumanAgent(Agent):
    
    def __init__(self, input_handler: InputHandler):
        self.input_handler = input_handler

    def get_action(self, observation: np.ndarray) -> Direction | None:
        return self.input_handler.direction