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

Position directionOffset(Direction dir) {
    switch (dir) {
        case Direction::UP:    return {0, -1};
        case Direction::DOWN:  return {0, 1};
        case Direction::LEFT:  return {-1, 0};
        case Direction::RIGHT: return {1, 0};
    }
    return {0, 0};
}

Direction vecToDir(Position offset) {
    if (offset.x == 0 && offset.y == -1) return Direction::UP;
    if (offset.x == 0 && offset.y == 1)  return Direction::DOWN;
    if (offset.x == -1 && offset.y == 0) return Direction::LEFT;
    if (offset.x == 1 && offset.y == 0)  return Direction::RIGHT;
    return Direction::UP;
}

std::vector<double> vectorMatrixMultiplication(const std::vector<double>& vec, const Matrix& mat) {
    std::vector<double> result(mat.cols, 0.0);

    for (int r = 0; r < mat.rows; r++) {
        const double vr = vec[static_cast<size_t>(r)];
        for (int c = 0; c < mat.cols; c++) {
            result[static_cast<size_t>(c)] += vr * mat(r, c);
        }
    }
    return result;
}

double computeRay(Position head, Position dir, Position boardSize, const std::array<bool, MAX_BOARD_CELLS>& mask) {
    for (int s = 1; ; ++s) {
        const int cx = head.x + dir.x * s;
        const int cy = head.y + dir.y * s;
        if (cx < 0 || cx >= boardSize.x ||
            cy < 0 || cy >= boardSize.y)
            return 0.0;
        if (mask[cy * boardSize.x + cx])
            return 1.0 / static_cast<double>(s);
    }
}

double computeWall(Position head, Position dir, Position boardSize) {
    int INF = boardSize.x * boardSize.y + 1;
    int sx  = (dir.x > 0) ? (boardSize.x - 1 - head.x) : ((dir.x < 0) ? head.x : INF);
    int sy  = (dir.y > 0) ? (boardSize.y - 1 - head.y) : ((dir.y < 0) ? head.y : INF);
    int s   = (sx < sy) ? sx : sy;
    return (s > 0) ? (1.0 / s) : 1.0;
}

std::vector<double> getObservation(const State& state, const std::vector<std::string>& components) {
    const int bw = state.boardSize.x;
    const int bh = state.boardSize.y;

    const Position fwd = directionOffset(state.direction);
    const Position rgt = fwd.rotateCW();
    const Position lft = fwd.rotateCCW();

    std::array<bool, MAX_BOARD_CELLS> mask = {};
    bool maskBuilt = false;
    auto ensureMask = [&]() {
        if (maskBuilt) return;
        for (size_t i = 1; i < state.snakePositions.size(); ++i) {
            const auto& p = state.snakePositions[i];
            mask[p.y * bw + p.x] = true;
        }
        maskBuilt = true;
    };

    auto blocked = [&](Position dir) -> double {
        const int cx = state.head.x + dir.x;
        const int cy = state.head.y + dir.y;
        if (cx < 0 || cx >= bw || cy < 0 || cy >= bh) return 1.0;
        ensureMask();
        return mask[cy * bw + cx] ? 1.0 : 0.0;
    };

    std::vector<double> out;
    for (const auto& c : components) {
        if (c == "relative_apple") {
            Position rel = state.apple - state.head;
            double afwd  = rel.x * fwd.x + rel.y * fwd.y;
            double aside = rel.x * rgt.x + rel.y * rgt.y;
            out.push_back((afwd  != 0.0) ? (1.0 / afwd)  : 0.0);
            out.push_back((aside != 0.0) ? (1.0 / aside) : 0.0);
        } else if (c == "absolute_apple") {
            out.push_back(static_cast<double>(state.apple.x) / (bw - 1));
            out.push_back(static_cast<double>(state.apple.y) / (bh - 1));
        } else if (c == "head_position") {
            out.push_back(static_cast<double>(state.head.x) / (bw - 1));
            out.push_back(static_cast<double>(state.head.y) / (bh - 1));
        } else if (c == "direction") {
            out.push_back(static_cast<double>(fwd.x));
            out.push_back(static_cast<double>(fwd.y));
        } else if (c == "snake_size") {
            out.push_back(static_cast<double>(state.snakePositions.size()) / (bw * bh));
        } else if (c == "distance_to_walls") {
            out.push_back(computeWall(state.head, fwd,      state.boardSize));
            out.push_back(computeWall(state.head, fwd * -1, state.boardSize));
            out.push_back(computeWall(state.head, rgt,      state.boardSize));
            out.push_back(computeWall(state.head, lft,      state.boardSize));
        } else if (c == "distance_to_danger") {
            ensureMask();
            out.push_back(computeRay(state.head, fwd, state.boardSize, mask));
            out.push_back(computeRay(state.head, rgt, state.boardSize, mask));
            out.push_back(computeRay(state.head, lft, state.boardSize, mask));
        } else if (c == "danger_flags") {
            out.push_back(blocked(fwd));
            out.push_back(blocked(rgt));
            out.push_back(blocked(lft));
        } else if (c == "full_grid") {
            // 0 empty, -1 body, 1 apple, 0.5 head
            std::vector<double> grid(bw * bh, 0.0);
            for (size_t i = 1; i < state.snakePositions.size(); ++i) {
                const auto& p = state.snakePositions[i];
                grid[p.y * bw + p.x] = -1.0;
            }
            grid[state.apple.y * bw + state.apple.x] = 1.0;
            grid[state.head.y * bw + state.head.x]   = 0.5;
            out.insert(out.end(), grid.begin(), grid.end());
        } else {
            assert(false && "unknown observation component");
        }
    }
    return out;
}