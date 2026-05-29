#include "icegod.h"
#include <QTransform>
#include <QPainter>

IceGod::IceGod(Player* target)
    : player(target)
{
    hp = fullHp;
    damage = 0;

    // 加载Boss本体贴图（竖6帧）
    QPixmap sheet(":/tu/ice god.png");
    if (!sheet.isNull()) {
        int fw = sheet.width();          // 250
        int fh = sheet.height() / 6;     // 210
        for (int i = 0; i < 6; i++) {
            QPixmap frame = sheet.copy(0, i * fh, fw, fh);
            frame = frame.scaled(frameSize, frameSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            normalFrames.push_back(frame);
        }
    }

    // 兜底
    if (normalFrames.isEmpty()) {
        QPixmap fallback(frameSize, frameSize);
        fallback.fill(Qt::transparent);
        QPainter p(&fallback);
        p.setBrush(QColor(100, 180, 255)); // 冰蓝色
        p.setPen(Qt::NoPen);
        p.drawEllipse(4, 4, frameSize - 8, frameSize - 8);
        p.end();
        normalFrames.push_back(fallback);
    }

    setPixmap(normalFrames[0]);
    setOffset((frameSize - normalFrames[0].width()) / 2.0,
              frameSize - normalFrames[0].height());
}

void IceGod::setFrame(QPixmap img, bool flipLeft) {
    if (flipLeft) {
        img = img.transformed(QTransform().scale(-1, 1));
    }
    setOffset((frameSize - img.width()) / 2.0, frameSize - img.height());
    setPixmap(img);
}

QPainterPath IceGod::shape() const {
    QPainterPath path;
    path.addRect(0, 0, frameSize, frameSize);
    return path;
}

void IceGod::takeDamage(int dmg) {
    int actualDmg = dmg;
    // 火形态伤害翻倍
    if (player && player->currentForm == Enemy::FIRE) {
        actualDmg *= 2;
    }
    Enemy::takeDamage(actualDmg);

    // 检查召唤波次
    int newWave = -1;
    if (summonWave == 0 && hp <= fullHp * 2 / 3) {
        newWave = 1;
    } else if (summonWave <= 1 && hp <= fullHp * 1 / 3) {
        newWave = 2;
    }
    if (newWave > summonWave) {
        summonWave = newWave;
        summonXuehuas(4);
    }
}

void IceGod::summonXuehuas(int count) {
    double angleStep = 2 * M_PI / count;
    for (int i = 0; i < count; i++) {
        Xuehua* x = new Xuehua(this, player);
        x->orbitAngle = angleStep * i;
        xuehuas.append(x);
        pendingXuehuas.append(x); // 主窗口从这里取，加入场景
    }
}

void IceGod::removeXuehua(Xuehua* x) {
    xuehuas.removeAll(x);
}

void IceGod::pickNewTarget() {
    if (!player) return;
    double offsetX = (rand() % 300 + 200) * (rand() % 2 == 0 ? 1 : -1);
    targetX = player->x() + offsetX;
    targetY = player->y() + (rand() % 400 - 200);
    if (targetX < frameSize) targetX = frameSize;
    if (targetX > sceneW - frameSize) targetX = sceneW - frameSize;
    if (targetY < frameSize) targetY = frameSize;
    if (targetY > sceneH - frameSize) targetY = sceneH - frameSize;
}

void IceGod::updateLogic() {
    if (isDead || !player) return;

    // 清理已死亡的雪花
    for (int i = xuehuas.size() - 1; i >= 0; i--) {
        if (xuehuas[i]->isDead) {
            xuehuas.removeAt(i);
        }
    }

    switch (state) {
    case FLYING: {
        damage = 0;

        if (flyTimer == 0) pickNewTarget();

        double dx = targetX - this->x();
        double dy = targetY - this->y();
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 5) {
            double speed = flySpeed;
            if (dist < 100) speed = flySpeed * (dist / 100.0);
            if (speed < 1.0) speed = 1.0;
            vx = (dx / dist) * speed;
            vy = (dy / dist) * speed;
        } else {
            vx = std::cos(flyTimer * 0.03) * 1.0;
            vy = std::sin(flyTimer * 0.05) * 1.5;
        }

        // 动画（放慢帧切换）
        animTimer++;
        if (animTimer >= 12) {
            animTimer = 0;
            currentFrame = (currentFrame + 1) % normalFrames.size();
            if (currentFrame < normalFrames.size())
                setFrame(normalFrames[currentFrame], vx < 0);
        }

        // 边界
        if (this->x() < 0) { this->setPos(0, this->y()); vx *= -1; }
        if (this->x() > sceneW - frameSize) { this->setPos(sceneW - frameSize, this->y()); vx *= -1; }
        if (this->y() < 0) { this->setPos(this->x(), 0); vy *= -1; }
        if (this->y() > sceneH - frameSize) { this->setPos(this->x(), sceneH - frameSize); vy *= -1; }

        flyTimer++;
        if (flyTimer >= flyDuration) {
            flyTimer = 0;
            state = SUMMONING;
            summonTimer = 0;
            vx = 0; vy = 0;
        }
        break;
    }

    case SUMMONING: {
        vx = 0; vy = 0;  // 悬停
        damage = 0;

        // 召唤姿态动画（放慢）
        animTimer++;
        if (animTimer >= 14) {
            animTimer = 0;
            currentFrame = (currentFrame + 1) % normalFrames.size();
            if (currentFrame < normalFrames.size()) {
                bool flip = (player->x() < this->x());
                setFrame(normalFrames[currentFrame], flip);
            }
        }

        summonTimer++;
        if (summonTimer >= summonDuration) {
            summonTimer = 0;
            state = FLYING;
            flyTimer = 0;
        }
        break;
    }
    }
}
