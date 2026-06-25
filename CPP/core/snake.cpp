#include "snake.h"

Snake::Snake(Position pos) {
    body.push_back({
        .pos = pos,
        .dir = Direction::RIGHT
    });
    occupiedPositions.insert(pos);
}

Position Snake::getHead() const {
    return body.front().pos;
}

Direction Snake::getDirection() const {
    return body.front().dir;
}

std::vector<Position> Snake::getPositions() const {
    std::vector<Position> p;
    p.reserve(body.size());
    for (const auto& unit : body) {
        p.push_back(unit.pos);
    }
    return p;
}

void Snake::move(std::optional<Direction> dir, bool apple) {
    Direction actualDir = dir.value_or(getDirection());
    if (isOpposite(actualDir, getDirection())) actualDir = getDirection();

    Position new_pos = getHead();

    switch(actualDir) {
        case Direction::UP:    new_pos.y--; break;
        case Direction::DOWN:  new_pos.y++; break;
        case Direction::LEFT:  new_pos.x--; break;
        case Direction::RIGHT: new_pos.x++; break;
    }

    selfCollision = occupiedPositions.count(new_pos) > 0;

    body.push_front({
        .pos = new_pos,
        .dir = actualDir
    });
    occupiedPositions.insert(new_pos);

    if (!apple) {
        occupiedPositions.erase(body.back().pos);
        body.pop_back();
    }
}

bool Snake::getCannibalism() const {
    return selfCollision;
}