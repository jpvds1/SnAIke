// helpers.y

#pragma once

#include <vector>

#include "types.h"

bool isOpposite(Direction dir1, Direction dir2);

std::vector<double> vectorMatrixMultiplication(const std::vector<double>& vec, const std::vector<std::vector<double>>& mat);

constexpr Position directionOffset(Direction dir)
{
    switch (dir) {
    case Direction::UP:    return {0, -1};
    case Direction::DOWN:  return {0, 1};
    case Direction::LEFT:  return {-1, 0};
    case Direction::RIGHT: return {1, 0};
    }
    return {0, 0};
}