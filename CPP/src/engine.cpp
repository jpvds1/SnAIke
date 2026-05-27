#include "../include/engine.h"

Engine::Engine() : snake({0, 0}) {
    width = 20;
    height = 20;
    state.boardSize = {width, height};
    reset();
}

State Engine::reset() {
    Position start;
    start.x = width / 2;
    start.y = height / 2;
    
    snake = Snake(start);

    state.apple = spawnApple().value_or({0, 0});
    state.score = 0;
    state.steps = 0;
    state.hunger = 0;
    state.done = false;
    return getState();
}

GymState Engine::update(std::optional<Direction> dir) {
    if (state.done) {
        return GymState{
            .state = getState(),
            .reward = 0.0f,
            .done = true
        };
    }

    Position nHead = nextHead(dir);
    bool ateApple = (nHead == state.apple);

    Direction actualDir = dir.value_or(snake.getDirection());
    if (isOpposite(actualDir, snake.getDirection())) {
        actualDir = snake.getDirection();
    }

    snake.move(actualDir, ateApple);

    state.steps++;
    state.hunger++;
    float stepReward = REWARD_STEP;

    if (ateApple) {
        state.score++;
        state.hunger = 0;
        state.apple = spawnApple().value_or({0, 0});
        stepReward += REWARD_APPLE;
    }

    if (isDead()) {
        state.done = true;
        stepReward += REWARD_DEATH;
    }

    if (state.hunger >= width * height) state.done = true;

    return GymState{
        .state = getState(),
        .reward = 0.0f,
        .done = true
    };
}

State Engine::getState() const {
    State current_state = state;
    current_state.snakePositions = snake.getPositions();
    current_state.head = snake.getHead();
    current_state.direction = snake.getDirection();
    return current_state;
}

Position Engine::nextHead(std::optional<Direction> dir) {
    Direction snakeDir = snake.getDirection();
    Direction finalDir = dir.value_or(snakeDir);

    if (isOpposite(finalDir, snakeDir)) {
        finalDir = snakeDir;
    }

    Position snakePos = snake.getHead();
    Position offsets = directionOffset(finalDir);
    return Position{
        .x = snakePos.x + offsets.x,
        .y = snakePos.y + offsets.y
    };
}

bool Engine::isDead() const {
    Position snakePos = snake.getHead();
    int x = snakePos.x;
    int y = snakePos.y;
    if (!(
        (0 <= x && x < width)
        &&
        (0 <= y && y < height)
    )) return true;

    return snake.getCannibalism();
}

std::optional<Position> Engine::spawnApple() {
    std::vector<Position> positions = snake.getPositions();
    std::unordered_set<Position, PositionHash> occupied(positions.begin(), positions.end());

    std::vector<Position> freeCells;
    freeCells.reserve(width * height);

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            Position p{x, y};

            if (!occupied.contains(p)) {
                freeCells.push_back(p);
            }
        }
    }

    if (freeCells.empty()) {
        return std::nullopt;
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<size_t> dist(0, freeCells.size() - 1);

    return freeCells[dist(gen)];
}