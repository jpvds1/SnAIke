#include "engine.h"
#include <cstddef>
#include <optional>
#include <random>
#include <vector>

Engine::Engine() : width(20), height(20), snake({width / 2, height / 2}), rng(std::random_device{}()) {
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

    const Position nHead = nextHead(dir);
    const bool ateApple = (nHead == state.apple);

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
    const Direction snakeDir = snake.getDirection();
    Direction finalDir = dir.value_or(snakeDir);

    if (isOpposite(finalDir, snakeDir)) {
        finalDir = snakeDir;
    }

    const Position head = snake.getHead();
    const Position offsets = directionOffset(finalDir);
    return {head.x + offsets.x, head.y + offsets.y};
}

bool Engine::isDead() const {
    const Position head = snake.getHead();
    if (head.x < 0 || head.x >= width ||
        head.y < 0 || head.y >= height)
        return true;

    return snake.getCannibalism();
}

std::optional<Position> Engine::spawnApple() {
    const std::vector<Position>& bodyPositions = snake.getPositions();
    const size_t snakeLen = bodyPositions.size();
    const size_t totalCells = static_cast<size_t>(width * height);

    if (snakeLen >= totalCells) return std::nullopt;

    std::uniform_int_distribution<int> dx(0, width - 1);
    std::uniform_int_distribution<int> dy(0, height - 1);

    if (snakeLen * 2 < totalCells) {
        Position p;
        do { 
            p = {dx(rng), dy(rng)}; 
        } while (std::find(bodyPositions.begin(), bodyPositions.end(), p) != bodyPositions.end());
        return p;
    }

    std::array<bool, 400> mask = {};
    for (const auto& p : bodyPositions)
        mask[static_cast<size_t>(p.y * width + p.x)] = true;

    std::vector<Position> freeCells;
    freeCells.reserve(totalCells - snakeLen);

    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            if (!mask[static_cast<size_t>(y * width + x)])
                freeCells.push_back({x, y});

    std::uniform_int_distribution<size_t> dist(0, freeCells.size() - 1);
    return freeCells[dist(rng)];
}