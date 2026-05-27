// types.h

#pragma once

#include <vector>
#include <cstddef>
#include <functional>

enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

struct Position {
    int x;
    int y;

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

struct PositionHash {
    std::size_t operator()(const Position& p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

struct BodyUnit {
    Position pos;
    Direction dir;
};

struct State {
    std::vector<Position> snakePositions;
    Position head;
    Position apple;
    Position boardSize;
    Direction direction;
    int score;
    int steps;
    int hunger;
    bool done;
};

struct GymState {
    State state;
    float reward;
    bool done;
};

struct RunResult {
    float totalReward;
    int score;
    int steps;
    bool aborted;
};