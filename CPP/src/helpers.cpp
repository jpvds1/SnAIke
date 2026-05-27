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