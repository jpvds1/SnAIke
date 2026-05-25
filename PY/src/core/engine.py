import random
from core.snake import Direction, Snake, _is_opposite

REWARD_APPLE     =  1.0
REWARD_DEATH     = -1.0
REWARD_STEP      =  0.0

_DIRECTION_OFFSETS: dict[Direction, tuple[int, int]] = {
    Direction.UP:    ( 0, -1),
    Direction.DOWN:  ( 0,  1),
    Direction.LEFT:  (-1,  0),
    Direction.RIGHT: ( 1,  0),
}

class Engine:
    def __init__(self):
        self.width = 20
        self.height = 20
        self.reset()

    # ---------------------------------------------
    # Public API
    # ---------------------------------------------
    
    def reset(self) -> dict:
        start = (self.width // 2, self.height // 2)
        self.snake = Snake(start_pos=start)
        self.apple = self._spawn_apple()
        self.score = 0
        self.steps = 0
        self.steps_since_apple = 0
        self.done = False
        return self.get_state()

    def update(self, direction: Direction | None = None) -> tuple[dict, float, bool]:
        if self.done:
            return self.get_state(), 0.0, True

        ate_apple = self._next_head(direction) == self.apple
        self.snake.move(direction, apple=ate_apple)
        
        self.steps += 1
        self.steps_since_apple += 1

        reward = REWARD_STEP

        if ate_apple:
            self.score += 1
            self.steps_since_apple = 0
            self.apple = self._spawn_apple()
            reward += REWARD_APPLE

        if self._is_dead():
            self.done = True
            reward += REWARD_DEATH

        # Prevent agent infinite loops
        if self.steps_since_apple >= self.width * self.height:
            self.done = True

        return self.get_state(), reward, self.done

    def get_state(self):
        return {
            "snake_positions": self.snake.positions,
            "head": self.snake.head.position,
            "direction": self.snake.head.direction,
            "apple": self.apple,
            "score": self.score,
            "steps": self.steps,
            "hunger": self.steps_since_apple,
            "done": self.done,
            "board_size": (self.width, self.height)
        }

    # ---------------------------------------------
    # Helpers
    # ---------------------------------------------

    def _next_head(self, direction: Direction | None):
        if direction is None or _is_opposite(direction, self.snake.head.direction):
            direction = self.snake.head.direction

        x, y = self.snake.head.position
        dx, dy = _DIRECTION_OFFSETS[direction]
        return (x + dx, y + dy)

    def _is_dead(self) -> bool:
        x, y = self.snake.head.position
        if not (0 <= x < self.width and 0 <= y < self.height):
            return True
        return self.snake.head.position in self.snake.positions[1:]

    def _spawn_apple(self) -> tuple[int, int] | None:
        occupied = set(self.snake.positions)
        free_cells = [
            (x, y)
            for x in range(self.width)
            for y in range(self.height)
            if (x, y) not in occupied
        ]
        return random.choice(free_cells) if free_cells else None