#include "brainofcthulhu.h"
#include <QTransform>
#include <QPainter>

BrainOfCthulhu::BrainOfCthulhu(Player* target)
    : player(target)
{
    hp = fullHp;
    damage = 0;

    // 加载Boss本体贴图（竖8帧：前4普通，后4二阶段）
    QPixmap sheet(":/tu/brainofcthlu.png");
    if (!sheet.isNull()) {
        int fw = sheet.width();
        int fh = sheet.height() / 8;
        for (int i = 0; i < 8; i++) {
            QPixmap frame = sheet.copy(0, i * fh, fw, fh);
            frame = frame.scaled(frameSize, frameSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            if (i < 4)
                normalFrames.push_back(frame);
            else
                phase2Frames.push_back(frame);
        }
    }

    // 兜底：如果图片加载失败，生成可见的占位贴图（紫色方块）
    if (normalFrames.isEmpty()) {
        for (int i = 0; i < 4; i++) {
            QPixmap fallback(frameSize, frameSize);
            fallback.fill(Qt::transparent);
            QPainter p(&fallback);
            p.setBrush(QColor(180, 50, 220)); // 紫色
            p.setPen(Qt::NoPen);
            p.drawEllipse(4, 4, frameSize - 8, frameSize - 8);
            p.end();
            normalFrames.push_back(fallback);
            phase2Frames.push_back(fallback);
        }
    }

    // 设置初始贴图
    setPixmap(normalFrames[0]);
    setOffset((frameSize - normalFrames[0].width()) / 2.0,
              frameSize - normalFrames[0].height());

    // 加载弹幕贴图（竖4帧），缩放到小尺寸
    int projSize = 16;
    QPixmap atkSheet(":/tu/cthulubrain_attack.png");
    if (!atkSheet.isNull()) {
        int fw = atkSheet.width();
        int fh = atkSheet.height() / 4;
        for (int i = 0; i < 4; i++) {
            QPixmap frame = atkSheet.copy(0, i * fh, fw, fh);
            frame = frame.scaled(projSize, projSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            attackFrames.push_back(frame);
        }
    }

    // 弹幕兜底：黄色小球
    if (attackFrames.isEmpty()) {
        QPixmap fallback(projSize, projSize);
        fallback.fill(Qt::transparent);
        QPainter p(&fallback);
        p.setBrush(QColor(255, 200, 50));
        p.setPen(Qt::NoPen);
        p.drawEllipse(0, 0, projSize, projSize);
        p.end();
        attackFrames.push_back(fallback);
    }
}

void BrainOfCthulhu::setFrame(QPixmap img, bool flipLeft) {
    if (flipLeft) {
        img = img.transformed(QTransform().scale(-1, 1));
    }
    setOffset((frameSize - img.width()) / 2.0, frameSize - img.height());
    setPixmap(img);
}

QPainterPath BrainOfCthulhu::shape() const {
    QPainterPath path;
    path.addRect(0, 0, frameSize, frameSize);
    return path;
}

void BrainOfCthulhu::takeDamage(int dmg) {
    Enemy::takeDamage(dmg);
    if (!isDead && dmg > 0) {
        invulnTimer = 8;
    }
    // 半血进入二阶段
    if (!isPhase2 && hp <= fullHp / 2) {
        isPhase2 = true;
    }
}

void BrainOfCthulhu::pickNewTarget() {
    if (!player) return;
    double offsetX = (rand() % 300 + 200) * (rand() % 2 == 0 ? 1 : -1);
    targetX = player->x() + offsetX;
    // 限制Y在玩家上下250px内（保持在屏幕可见范围）
    targetY = player->y() + (rand() % 500 - 250);
    // 边界钳制
    if (targetX < frameSize) targetX = frameSize;
    if (targetX > sceneW - frameSize) targetX = sceneW - frameSize;
    if (targetY < frameSize) targetY = frameSize;
    if (targetY > sceneH - frameSize) targetY = sceneH - frameSize;
}

void BrainOfCthulhu::updateLogic() {
    if (isDead || !player) return;

    // 受伤闪烁
    if (invulnTimer > 0) {
        invulnTimer--;
        setVisible(invulnTimer % 2 == 0);
    } else {
        setVisible(true);
    }

    // 二阶段飞行加速
    double currentFlySpeed = isPhase2 ? flySpeed * 1.4 : flySpeed;

    switch (state) {
    // ========== 飞行 ==========
    case FLYING: {
        damage = 0;

        if (flyTimer == 0) pickNewTarget();

        double dx = targetX - this->x();
        double dy = targetY - this->y();
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 5) {
            double speed = currentFlySpeed;
            if (dist < 100) speed = currentFlySpeed * (dist / 100.0);
            if (speed < 1.5) speed = 1.5;
            vx = (dx / dist) * speed;
            vy = (dy / dist) * speed;
        } else {
            // 到达目标附近，小范围漂移
            vx = std::cos(flyTimer * 0.05) * 1.5;
            vy = std::sin(flyTimer * 0.07) * 2.0;
        }

        // 动画
        animTimer++;
        const QVector<QPixmap>& curFrames = isPhase2 ? phase2Frames : normalFrames;
        if (animTimer >= 5) {
            animTimer = 0;
            currentFrame = (currentFrame + 1) % curFrames.size();
            if (currentFrame < curFrames.size())
                setFrame(curFrames[currentFrame], vx < 0);
        }

        // 边界
        if (this->x() < 0) { this->setPos(0, this->y()); vx *= -1; }
        if (this->x() > sceneW - frameSize) { this->setPos(sceneW - frameSize, this->y()); vx *= -1; }
        if (this->y() < 0) { this->setPos(this->x(), 0); vy *= -1; }
        if (this->y() > sceneH - frameSize) { this->setPos(this->x(), sceneH - frameSize); vy *= -1; }

        flyTimer++;
        if (flyTimer >= flyDuration) {
            flyTimer = 0;
            state = ATTACKING;
            attackTimer = 0;
            shootTimer = 0;
            vx = 0; vy = 0;
            damage = 1;
        }
        break;
    }

    // ========== 攻击（悬停+8方向弹幕）==========
    case ATTACKING: {
        vx = 0; vy = 0;  // 悬停
        damage = 1;

        // 攻击动画：使用二阶段或普通的第一帧（张嘴帧）
        const QVector<QPixmap>& curFrames = isPhase2 ? phase2Frames : normalFrames;
        // 挥动翅膀的动画
        animTimer++;
        if (animTimer >= 5) {
            animTimer = 0;
            currentFrame = (currentFrame + 1) % curFrames.size();
            if (currentFrame < curFrames.size()) {
                bool flip = (player->x() < this->x());
                setFrame(curFrames[currentFrame], flip);
            }
        }

        // 发射弹幕
        shootTimer++;
        // 二阶段射击更快
        int interval = isPhase2 ? shootInterval / 2 : shootInterval;   // 二阶段0.5秒变0.25秒
        if (shootTimer >= interval && !attackFrames.isEmpty()) {
            shootTimer = 0;

            // 8个方向
            const double angles[8] = {
                0.0,                     // 右
                -M_PI / 4.0,             // 右上
                -M_PI / 2.0,             // 上
                -3.0 * M_PI / 4.0,       // 左上
                M_PI,                     // 左
                3.0 * M_PI / 4.0,         // 左下
                M_PI / 2.0,              // 下
                M_PI / 4.0               // 右下
            };

            double speed = 4.0;
            for (double angle : angles) {
                Projectile* p = new Projectile(
                    attackFrames[0],           // 初始贴图
                    std::cos(angle) * speed,   // vx
                    std::sin(angle) * speed,   // vy
                    120,                        // 寿命2秒
                    1                           // 伤害
                );
                p->animFrames = attackFrames;  // 设置动画帧
                p->hurtsEnemies = false;
                p->hurtsPlayer = true;
                // 从Boss碰撞箱边缘生成
                double spawnDist = frameSize / 2.0 + 4;
                double halfProj = attackFrames[0].width() / 2.0;
                p->setPos(this->x() + frameSize / 2.0 + std::cos(angle) * spawnDist - halfProj,
                          this->y() + frameSize / 2.0 + std::sin(angle) * spawnDist - halfProj);
                pendingProjectiles.append(p);
            }
        }

        attackTimer++;
        if (attackTimer >= attackDuration) {
            attackTimer = 0;
            state = FLYING;
            flyTimer = 0;
            vx = 0; vy = 0;
            damage = 0;
        }
        break;
    }
    }
}
