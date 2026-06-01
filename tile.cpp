#include "tile.h"
#include "player.h"
#include <cmath>
#include <QGraphicsScene>

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

void Tile::resetAmbush() {
    ambushArmed = false;
    ambushDelay = 0;
    ambushChargeTimer = 0;
    ambushDir = 1;
}

void Tile::updateLogic() {
    // ====== 伏击刺逻辑 ======
    if (m_type == AmbushSpike) {
        if (!ambushOriginSet) {
            ambushHomeX = x();
            ambushHomeY = y();
            ambushOriginSet = true;
        }

        if (!ambushArmed) {
            // 检测玩家：遍历场景找Player
            if (!scene()) return;
            QList<QGraphicsItem*> items = scene()->items();
            Player* foundPlayer = nullptr;
            for (QGraphicsItem* item : items) {
                Player* p = dynamic_cast<Player*>(item);
                if (p) { foundPlayer = p; break; }
            }
            if (!foundPlayer) return;

            qreal dx = foundPlayer->x() - x();
            qreal dy = foundPlayer->y() - y();
            qreal dist = std::abs(dx);

            // 横向300px内 + 纵向48px内 → 触发
            if (dist < 300 && std::abs(dy) < 48) {
                ambushArmed = true;
                ambushDelay = 30; // 0.5秒预警
                ambushDir = (dx > 0) ? 1 : -1;
            }
            return;
        }

        // 预警期：抖动
        if (ambushDelay > 0) {
            ambushDelay--;
            double shake = std::sin(ambushDelay * 0.5) * 2.0;
            setPos(ambushHomeX + shake, ambushHomeY);
            return;
        }

        // 冲刺期
        if (ambushChargeTimer == 0) {
            double offset = ambushDir > 0 ? x() - ambushHomeX : ambushHomeX - x();
            if (offset < ambushMaxRange) {
                setPos(x() + 6.0 * ambushDir, y());
            } else {
                ambushChargeTimer = 45; // 停顿
            }
            return;
        }

        // 停顿后回缩
        if (ambushChargeTimer > 0) {
            ambushChargeTimer--;
            return;
        }

        // 回缩到原点
        double toHome = std::abs(x() - ambushHomeX);
        if (toHome < 2.0) {
            setPos(ambushHomeX, ambushHomeY);
            resetAmbush();
        } else {
            int backDir = (x() > ambushHomeX) ? -1 : 1;
            setPos(x() + 2.0 * backDir, y());
        }
        return;
    }

    // ====== 原有移动刺逻辑 ======
    if (!canMove) return;

    // 首次更新时记录原点
    if (!originSet) {
        moveOriginX = x();
        moveOriginY = y();
        originSet = true;
    }

    switch (moveAxis) {
    case Horizontal: {
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
