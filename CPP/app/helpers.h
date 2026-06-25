// helpers.h

#pragma once

#include <vector>
#include <string>
#include <cstddef>
#include <cassert>

#include "types.h"

static constexpr int MAX_BOARD_CELLS = 400;

bool isOpposite(Direction dir1, Direction dir2);
Position directionOffset(Direction dir);
Direction vecToDir(Position offset);

std::vector<double> vectorMatrixMultiplication(const std::vector<double>& vec, const Matrix& mat);

double computeRay(Position head, Position dir, Position boardSize, const std::array<bool, MAX_BOARD_CELLS>& mask);
double computeWall(Position head, Position dir, Position boardSize);

std::vector<double> getObservation(const State& state, const std::vector<std::string>& components);

int observationSize(const std::vector<std::string>& components);