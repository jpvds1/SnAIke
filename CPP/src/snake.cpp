#include "snake.h"

Snake::Snake(Position pos) {
    return;
}

Position Snake::getHead() const {
    return {0, 0};
}

Direction Snake::getDirection() const {
    return Direction::UP;
}

std::vector<Position> Snake::getPositions() const {
    std::vector<Position> p;
    return p;
}

void Snake::move(Direction dir, bool apple) {
    return;
}

bool Snake::getCannibalism() const {
    return false;
}