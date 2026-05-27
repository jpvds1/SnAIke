// snake.h

#pragma once

#include "types.h"

class Snake {
public:
    Snake(Position pos);

    Position getHead() const;
    Direction getDirection() const;
    std::vector<Position> getPositions() const;
    void move(Direction dir, bool apple);
    bool getCannibalism() const;
};