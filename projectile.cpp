#include "projectile.h"
#include <QPainter>

Projectile::Projectile(bool movingRight) {
    // 动态绘制一个黄色的光球（如果你有图片，这里可以换成加载QPixmap）
    QPixmap pix(24, 24);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setBrush(Qt::yellow);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(0, 0, 24, 24);
    setPixmap(pix);

    // 设置速度，向右为正，向左为负
    vx = movingRight ? 12 : -12;
}

Projectile::Projectile(const QPixmap &pix, double velX, double velY, int lifetime, int dmg)
{
    setPixmap(pix);
    vx = velX;
    vy = velY;
    lifeTime = lifetime;
    damage = dmg;
}

void Projectile::updateLogic() {
    lifeTime--; // 寿命减少

    // 重力加速
    if (hasGravity) {
        vy += 0.4;
        if (vy > 12) vy = 12;
    }

    setPos(x() + vx, y() + vy); // 移动

    // 动画帧更新
    if (!animFrames.isEmpty()) {
        animTimer++;
        if (animTimer >= 6) { // 每6帧切换一张
            animTimer = 0;
            animFrameIdx = (animFrameIdx + 1) % animFrames.size();
            setPixmap(animFrames[animFrameIdx]);
        }
    }
}