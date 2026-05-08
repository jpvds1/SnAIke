from enum import Enum, auto

class Direction(Enum):
    UP    = auto()
    DOWN  = auto()
    LEFT  = auto()
    RIGHT = auto()

class BodyUnit:
    def __init__(self, direction: Direction, position: (int, int)):
        self.position = position
        self.direction = direction

class Snake:
    def __init__(self, start_pos = (0, 0)):
        self.body: list(BodyUnit) = [BodyUnit(Direction.RIGHT, start_pos)]
  
    @property
    def head(self) -> BodyUnit:
        return self.body[0]

    @property
    def positions(self) -> list[tuple[int, int]]:
        return [unit.position for unit in self.body]

    def move(self, direction: Direction | None = None, apple: bool = False):
        if direction == None:
            direction = self.head.direction
        if _is_opposite(direction, head.direction):
            direction = self.head.direction

        x, y = self.head.position

        if direction == Direction.UP:
            new_pos = (x, y - 1)
        elif direction == Direction.DOWN:
            new_pos = (x, y + 1)
        elif direction == Direction.LEFT:
            new_pos = (x - 1, y)
        elif direction == Direction.RIGHT:
            new_pos = (x + 1, y)

        self.body.insert(0, BodyUnit(direction, new_pos))

        if not apple:
            self.body.pop()


def _is_opposite(dir1: Direction, dir2: Direction) -> bool:
    pairs = {
        (Direction.UP,    Direction.DOWN),
        (Direction.DOWN,  Direction.UP),
        (Direction.LEFT,  Direction.RIGHT),
        (Direction.RIGHT, Direction.LEFT),
    }
    return (dir1, dir2) in pairs