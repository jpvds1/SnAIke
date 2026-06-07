// snake.h

#pragma once

#include <optional>

#include "types.h"
#include "helpers.h"

class Snake {
public:
    Snake(Position pos);

    Position getHead() const;
    Direction getDirection() const;
    std::vector<Position> getPositions() const;
    
    void move(std::optional<Direction> dir, bool apple);
    bool getCannibalism() const;
private:
    std::vector<BodyUnit> body;
};