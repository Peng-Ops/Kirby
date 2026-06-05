#include "badge.h"
#include <QPainter>

Badge::Badge(BadgeType type)
    : badgeType(type)
{
    QString path;
    switch (type) {
    case PIG_SHARK: path = ":/tu/zhushahuizhang.png"; break;
    case BRAIN:     path = ":/tu/naohuizhang.png";    break;
    case ICE_QUEEN: path = ":/tu/nvwanghuizhang.png"; break;
    }

    QPixmap pix(path);
    if (!pix.isNull()) {
        setPixmap(pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        // 兜底：金色方块
        QPixmap fallback(48, 48);
        fallback.fill(Qt::transparent);
        QPainter p(&fallback);
        p.setBrush(QColor(255, 215, 0));
        p.setPen(Qt::NoPen);
        p.drawEllipse(2, 2, 44, 44);
        p.end();
        setPixmap(fallback);
    }

    setZValue(500);
}

void Badge::updateLogic() {
    // 重力
    if (!isResting) {
        vy += gravity;
        if (vy > maxVy) vy = maxVy;
        setPos(x() + vx, y() + vy);
    }
}
