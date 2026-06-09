// snake.h

#pragma once

#include <optional>
#include <deque>
#include <unordered_set>

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
    std::deque<BodyUnit> body;
    std::unordered_set<Position, PositionHash> occupiedPositions;
    bool selfCollision = false;
};