#include "apple.h"
#include "player.h"
#include <QGraphicsScene>
#include <QFont>
#include <cmath>

Apple::Apple() {
    hp = 999;       // 不可杀死
    damage = 999;   // 秒杀
    setScale(baseScale);

    QPixmap pix(":/tu/apple.png");
    if (pix.isNull()) {
        pix = QPixmap(48, 48);
        pix.fill(QColor(200, 50, 50));
    }
    setPixmap(pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void Apple::updateLogic() {
    if (!scene()) return;

    // 找玩家
    Player* foundPlayer = nullptr;
    QList<QGraphicsItem*> items = scene()->items();
    for (QGraphicsItem* item : items) {
        Player* p = dynamic_cast<Player*>(item);
        if (p) { foundPlayer = p; break; }
    }

    switch (state) {
    case IDLE: {
        if (!foundPlayer) return;
        qreal dx = foundPlayer->x() - x();
        qreal dy = foundPlayer->y() - y();
        if (std::abs(dx) < 200 && std::abs(dy) < 100) {
            state = WARNING;
            warningTimer = 60;
            // 创建预警感叹号
            warningIndicator = new QGraphicsTextItem("!");
            warningIndicator->setFont(QFont("SimHei", 28, QFont::Bold));
            warningIndicator->setDefaultTextColor(QColor(255, 50, 50));
            warningIndicator->setZValue(2000);
            warningIndicator->setPos(x() + 8, y() - 40);
            scene()->addItem(warningIndicator);
        }
        break;
    }
    case WARNING: {
        warningTimer--;
        // 闪烁效果
        if (warningIndicator) {
            warningIndicator->setVisible(warningTimer % 10 < 5);
            warningIndicator->setPos(x() + 8, y() - 40);
        }
        // 缓慢追踪玩家
        if (foundPlayer) {
            qreal dx = foundPlayer->x() - x();
            qreal dy = foundPlayer->y() - y();
            qreal dist = std::sqrt(dx*dx + dy*dy);
            if (dist > 1) {
                setPos(x() + dx / dist * 1.5, y() + dy / dist * 1.5);
            }
        }
        if (warningTimer <= 0) {
            if (warningIndicator) {
                scene()->removeItem(warningIndicator);
                delete warningIndicator;
                warningIndicator = nullptr;
            }
            state = GROWING;
            growTimer = 120;
        }
        break;
    }
    case GROWING: {
        growTimer--;
        qreal t = 1.0 - (qreal)growTimer / 120.0;
        qreal s = baseScale + (maxScale - baseScale) * t;
        setScale(s);
        // 继续追踪玩家（速度更快）
        if (foundPlayer) {
            qreal dx = foundPlayer->x() - x();
            qreal dy = foundPlayer->y() - y();
            qreal dist = std::sqrt(dx*dx + dy*dy);
            if (dist > 1) {
                setPos(x() + dx / dist * 1.0, y() + dy / dist * 1.0);
            }
        }
        // 变大时检测碰撞，秒杀
        if (foundPlayer && collidesWithItem(foundPlayer)) {
            foundPlayer->hp = 0;
        }
        if (growTimer <= 0) {
            pauseTimer = 60;
            state = SHRINKING;
        }
        break;
    }
    case SHRINKING: {
        if (pauseTimer > 0) {
            pauseTimer--;
            // 停顿期间继续追踪
            if (foundPlayer) {
                qreal dx = foundPlayer->x() - x();
                qreal dy = foundPlayer->y() - y();
                qreal dist = std::sqrt(dx*dx + dy*dy);
                if (dist > 1) {
                    setPos(x() + dx / dist * 0.8, y() + dy / dist * 0.8);
                }
            }
            if (foundPlayer && collidesWithItem(foundPlayer)) {
                foundPlayer->hp = 0;
            }
            return;
        }
        shrinkTimer++;
        qreal t = (qreal)shrinkTimer / 60.0;
        if (t >= 1.0) {
            t = 1.0;
            state = IDLE;
            shrinkTimer = 0;
            setScale(baseScale);
        } else {
            qreal s = maxScale - (maxScale - baseScale) * t;
            setScale(s);
        }
        // 缩小过程中缓慢追踪
        if (foundPlayer) {
            qreal dx = foundPlayer->x() - x();
            qreal dy = foundPlayer->y() - y();
            qreal dist = std::sqrt(dx*dx + dy*dy);
            if (dist > 1) {
                setPos(x() + dx / dist * 1.2, y() + dy / dist * 1.2);
            }
        }
        if (foundPlayer && collidesWithItem(foundPlayer)) {
            foundPlayer->hp = 0;
        }
        break;
    }
    }
}
