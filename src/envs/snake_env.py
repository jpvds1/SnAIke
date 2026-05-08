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
        return self._build_obs(self._last_state)

    def step(self, action: Direction | None) -> tuple[np.ndarray, float, bool, dict]:
        state, reward, done = self.engine.update(action)
        self._last_state = state
        return self._build_obs(state), reward, done, state

    def get_observation(self):
        return self._build_obs(self._last_state)

    # ------------------------------------------------------------------
    # Internal
    # ------------------------------------------------------------------

    def _build_obs(self, state: dict) -> np.ndarray:
        danger    = state["danger"]
        direction = state["direction"]
        dx, dy    = state["delta_apple"]
        bw, bh    = state["board_size"]

        return np.array([
            float(danger["ahead"]),
            float(danger["left"]),
            float(danger["right"]),
            float(direction == Direction.UP),
            float(direction == Direction.DOWN),
            float(direction == Direction.LEFT),
            float(direction == Direction.RIGHT),
            dx / bw,
            dy / bh
        ], dtype=np.float32)