import random
from core.snake import Direction, Snake, _is_opposite

REWARD_APPLE     =  1.0
REWARD_DEATH     = -1.0
REWARD_STEP      =  0.0

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
        head = self.snake.head.position
        head_dir = self.snake.head.direction
        apple = self.apple

        return {
            "snake_positions": self.snake.positions,
            "head": head,
            "direction": head_dir,
            "apple": apple,
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
        return {
            Direction.UP:    (x, y - 1),
            Direction.DOWN:  (x, y + 1),
            Direction.LEFT:  (x - 1, y),
            Direction.RIGHT: (x + 1, y),
        }[direction]

    def _is_dead(self) -> bool:
        head = self.snake.head.position
        x, y = head
        out_of_bounds = not (0 <= x < self.width and 0 <= y < self.height)
        self_collision = head in self.snake.positions[1:]
        return out_of_bounds or self_collision

    def _spawn_apple(self) -> tuple[int, int] | None:
        occupied = set(self.snake.positions)
        free_cells = [
            (x, y)
            for x in range(self.width)
            for y in range(self.height)
            if (x, y) not in occupied
        ]
        return random.choice(free_cells) if free_cells else None