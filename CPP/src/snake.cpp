#include "snake.h"

Snake::Snake(Position pos) {
    body.push_back({
        .pos = pos,
        .dir = Direction::RIGHT
    });
}

Position Snake::getHead() const {
    return body.front().pos;
}

Direction Snake::getDirection() const {
    return body.front().dir;
}

std::vector<Position> Snake::getPositions() const {
    std::vector<Position> p;
    for (auto unit : body) {
        p.push_back(unit.pos);
    }
    return p;
}

void Snake::move(std::optional<Direction> dir, bool apple) {
    Direction actualDir = dir.value_or(getDirection());
    if (isOpposite(actualDir, getDirection())) actualDir = getDirection();

    Position new_pos = getHead();

    switch(actualDir) {
        case Direction::UP:
            new_pos.y--;
            break;
        case Direction::DOWN:
            new_pos.y++;
            break;
        case Direction::LEFT:
            new_pos.x--;
            break;
        case Direction::RIGHT:
            new_pos.x++;
            break;
    }

    body.push_back({
        .pos = new_pos,
        .dir = actualDir
    });

    if (!apple) {
        body.pop_back();
    }
}

bool Snake::getCannibalism() const {
    Position head = getHead();

    for (auto unit : body) {
        if (head == unit.pos) {
            return true;
        }
    }

    return false;
}