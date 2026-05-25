import numpy as np
from core.engine import Engine
from core.snake import Direction

class SnakeEnv:
    ACTIONS: list[Direction] = list(Direction)

    def __init__(self, width: int = 20, height = 20):
        self.engine = Engine()
        self.engine.width = width
        self.engine.height = height
        self._last_state: dict = {}

    # ------------------------------------------------------------------
    # Gym-style interface
    # ------------------------------------------------------------------


    def reset(self) -> np.ndarray:
        self._last_state = self.engine.reset()
        return self._last_state

    def step(self, action: Direction | None) -> tuple[np.ndarray, float, bool, dict]:
        state, reward, done = self.engine.update(action)
        self._last_state = state
        return state, reward, done, state