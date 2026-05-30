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

std::vector<double> vectorMatrixMultiplication(const std::vector<double>& vec, const std::vector<std::vector<double>>& mat) {
    size_t rows = mat.size();
    size_t cols = mat[0].size();
    std::vector<double> result(cols, 0.0);

    for (size_t c = 0; c < cols; c++) {
        for (size_t r = 0; r < rows; r++) {
            result[c] += vec[r] * mat[r][c];
        }
    }
    return result;
}