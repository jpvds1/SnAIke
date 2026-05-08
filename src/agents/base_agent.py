from abc import ABC, abstractmethod
import numpy as np
from core.snake import Direction

class Agent(ABC):
    @abstractmethod
    def get_action(self, observation: np.ndarray) -> Direction | None:
        raise NotImplementedError