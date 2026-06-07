#include "helpers.h"

bool isOpposite(Direction dir1, Direction dir2) {
    switch (dir1) {
        case Direction::UP:    return dir2 == Direction::DOWN;
        case Direction::DOWN:  return dir2 == Direction::UP;
        case Direction::LEFT:  return dir2 == Direction::RIGHT;
        case Direction::RIGHT: return dir2 == Direction::LEFT;
        default:               return false;
    }
}

constexpr Position directionOffset(Direction dir) {
    switch (dir) {
        case Direction::UP:    return {0, -1};
        case Direction::DOWN:  return {0, 1};
        case Direction::LEFT:  return {-1, 0};
        case Direction::RIGHT: return {1, 0};
    }
    return {0, 0};
}

std::vector<double> vectorMatrixMultiplication(const std::vector<double>& vec, const Matrix& mat) {
    std::vector<double> result(mat.cols, 0.0);

    for (size_t c = 0; c < mat.cols; c++) {
        for (size_t r = 0; r < mat.rows; r++) {
            result[c] += vec[r] * mat(r, c);
        }
    }
    return result;
}

Direction vecToDir(Position offset) {
    if (offset.x == 0 && offset.y == -1) return Direction::UP;
    if (offset.x == 0 && offset.y == 1)  return Direction::DOWN;
    if (offset.x == -1 && offset.y == 0) return Direction::LEFT;
    if (offset.x == 1 && offset.y == 0)  return Direction::RIGHT;
    return Direction::UP;
}

double computeRay(Position head, Position dir, Position boardSize, std::vector<Position>& snakePositions) {
    int s = 1;
    while (true) {
        Position checkPos = head + (dir * s);
        if (checkPos.x < 0 || checkPos.x >= boardSize.x || checkPos.y < 0 || checkPos.y >= boardSize.y) {
            return 0.0;
        }

        bool isOccupied = false;
        for (int i = 1; i < snakePositions.size(); ++i) {
            if (snakePositions[i] == checkPos) {
                isOccupied = true;
                break;
            }
        }
        if (isOccupied) {
            return 1.0 / s;
        }
        s++;
    }
}

double computeWall(Position head, Position dir, Position boardSize) {
    int INF = boardSize.x * boardSize.y + 1;
    int sx = (dir.x > 0) ? (boardSize.x - 1 - head.x) : ((dir.x < 0) ? head.x : INF);
    int sy = (dir.y > 0) ? (boardSize.y - 1 - head.y) : ((dir.y < 0) ? head.y : INF);
    int s = (sx < sy) ? sx : sy;
    return (s > 0) ? (1.0 / s) : 1.0;
}

std::vector<double> getObservation(const State& state) {
    double snake_size = static_cast<double>(state.snakePositions.size()) / (state.boardSize.x * size.boardSize.y);

    Position fwdVec = directionOffset(state.direction);
    Position rgtVec = fwdVec.rotateCW();
    Position lftVec = fwdVec.rotateCCW();

    double ba = computeRay(state.head, fwdVec, state.boardSize, state.snakePositions);
    double br = computeRay(state.head, rgtVec, state.boardSize, state.snakePositions);
    double bl = computeRay(state.head, lftVec, state.boardSize, state.snakePositions);

    double wa = computeWall(state.head, fwdVec, state.boardSize);
    double wb = computeWall(state.head, fwdVec * -1, state.boardSize);
    double wr = computeWall(state.head, rgtVec, state.boardSize);
    double wl = computeWall(state.head, lftVec, state.boardSize);

    Position relApple = state.apple - state.head;
    double afwd = relApple.x * fwdVec.x + relApple.y * fwdVec.y;
    double aside = relApple.x * rgtVec.x + relApple.y * rgtVec.y;
    double av = (afwd != 0.0) ? (1.0 / afwd) : 0.0;
    double asv = (aside != 0.0) ? (1.0 / aside) : 0.0;

    return {ba, br, bl, wa, wb, wr, wl, static_cast<double>(fwdVec.x), static_cast<double>(fwdVec.y), av, asv, snake_size};
}