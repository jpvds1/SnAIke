#include "../include/engine.h"

Engine::Engine() : width(20), height(20), snake({width / 2, height / 2}) {
    reset();
}

State Engine::reset() {
    Position start{width / 2, height / 2};
    snake = Snake(start);

    state = State{};
    state.boardSize = {width, height};
    state.apple = spawnApple().value_or({0, 0});
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
        .reward = stepReward,
        .done = state.done
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

    Position head = snake.getHead();
    Position offsets = directionOffset(finalDir);
    return {head.x + offsets.x, head.y + offsets.y};
}

bool Engine::isDead() const {
    Position head = snake.getHead();
    if (head.x < 0 || head.x >= width ||
        head.y < 0 || head.y >= height)
        return true;

    return snake.getCannibalism();
}

std::optional<Position> Engine::spawnApple() {
    std::vector<Position> positions = snake.getPositions();
    std::unordered_set<Position, PositionHash> occupied(positions.begin(), positions.end());

    std::vector<Position> freeCells;
    freeCells.reserve(width * height);

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            if (!occupied.count({x, y})) {
                freeCells.push_back({x, y});
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