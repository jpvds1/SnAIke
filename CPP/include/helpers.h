// helpers.y

#pragma once

#include <vector>

#include "types.h"

bool isOpposite(Direction dir1, Direction dir2);
constexpr Position directionOffset(Direction dir)

std::vector<double> vectorMatrixMultiplication(const std::vector<double>& vec, const Matrix& mat);

Direction vecToDir(Position offset);

double computeRay(Position head, Position dir, Position boardSize, const std::vector<Position>& snakePositions);
double computeWall(Position head, Position dir, Position boardSize);
std::vector<double> getObservation(const State& state);