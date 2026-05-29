#include "tile.h"
#include <cmath>

Tile::Tile(TileType type, const QPixmap &pixmap)
    : m_type(type)
{
    setPixmap(pixmap);
    vx = 0;
    vy = 0;
}

void Tile::enableMove(MoveAxis axis, double speed, double range) {
    canMove = true;
    moveAxis = axis;
    moveSpeed = speed;
    moveRange = range;
}

void Tile::updateLogic() {
    if (!canMove) return;

    // 首次更新时记录原点
    if (!originSet) {
        moveOriginX = x();
        moveOriginY = y();
        originSet = true;
    }

    switch (moveAxis) {
    case Horizontal: {
        // 水平往返
        double offset = x() - moveOriginX;
        if (offset >= moveRange) {
            moveDirection = -1;
        } else if (offset <= -moveRange) {
            moveDirection = 1;
        }
        setPos(x() + moveSpeed * moveDirection, y());
        break;
    }
    case Vertical: {
        // 垂直往返
        double offset = y() - moveOriginY;
        if (offset >= moveRange) {
            moveDirection = -1;
        } else if (offset <= -moveRange) {
            moveDirection = 1;
        }
        setPos(x(), y() + moveSpeed * moveDirection);
        break;
    }
    default:
        break;
    }
}

void Tile::changeType(TileType newType, const QPixmap &newPix) {
    m_type = newType;
    setPixmap(newPix);
}