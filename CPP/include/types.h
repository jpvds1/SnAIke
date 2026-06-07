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

struct Vector2D {
    int x;
    int y;

    bool operator==(const Vector2D& other) const { return x == other.x && y == other.y; }
    bool operator!=(const Vector2D& other) const { return !(*this == other); }

    Vector2D operator+(const Vector2D& o) const {return {x + o.x, y + o.y}; }
    Vector2D operator-(const Vector2D& o) const {return {x - o.x, y - o.y}; }
    Vector2D operator*(int s)             const {return {x * s, y * s}; }

    Vector2D rotateCW() const { return {-y, x}; }
    Vector2D rotateCCW() const { return {y, -x}; }

    int manhattanDistance(const Vector2D& other) const {
        return std::abs(x - other.x) + std::abs(y - other.y);
    }
};

using Position = Vector2D;

struct PositionHash {
    std::size_t operator()(const Position& p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 1);
    }
};

struct Matrix {
    int rows;
    int cols;
    std::vector<double> data;

    Matrix() : rows(0), cols(0) {}
    Matrix(int r, int c) : rows(r), cols(c), data(r * c, 0.0) {}
    double& operator()(int r, int c) { return data[r * cols + c]; }
    const double& operator()(int r, int c) const { return data[r * cols + c]; }
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