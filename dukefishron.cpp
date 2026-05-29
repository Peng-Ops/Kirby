#include "dukefishron.h"
#include <QTransform>

DukeFishron::DukeFishron(Player* target)
    : player(target)
{
    hp = fullHp;
    damage = 0;

    QPixmap spriteSheet(":/tu/pig_shark.png");
    if (!spriteSheet.isNull()) {
        int fw = spriteSheet.width();
        int fh = spriteSheet.height() / 8;
        for (int i = 0; i < 8; i++) {
            QPixmap frame = spriteSheet.copy(0, i * fh, fw, fh);
            frame = frame.scaled(frameSize, frameSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            allFrames.push_back(frame);
        }
        if (!allFrames.isEmpty()) {
            QPixmap img = allFrames[0];
            setOffset((frameSize - img.width()) / 2.0, frameSize - img.height());
            setPixmap(img);
        }
    }
}

void DukeFishron::setFrame(QPixmap img, bool flipLeft) {
    if (flipLeft) {
        img = img.transformed(QTransform().scale(-1, 1));
    }
    setOffset((frameSize - img.width()) / 2.0, frameSize - img.height());
    setPixmap(img);
}

QPainterPath DukeFishron::shape() const {
    QPainterPath path;
    path.addRect(0, 0, frameSize, frameSize);
    return path;
}

void DukeFishron::takeDamage(int dmg) {
    Enemy::takeDamage(dmg);
    if (!isDead && dmg > 0) {
        invulnTimer = 8;       // 8帧闪烁
    }
}

void DukeFishron::pickNewTarget() {
    if (!player) return;
    // X: 在卡比左右200~500px范围
    double offsetX = (rand() % 300 + 200) * (rand() % 2 == 0 ? 1 : -1);
    targetX = player->x() + offsetX;
    // Y: 全图随机高度，不依赖卡比——猪鲨上下穿梭
    targetY = (double)(rand() % ((int)sceneH - frameSize));
    if (targetX < 0) targetX = 0;
    if (targetX > sceneW - frameSize) targetX = sceneW - frameSize;
}

void DukeFishron::updateLogic() {
    if (isDead || !player) return;

    // 受伤闪烁
    if (invulnTimer > 0) {
        invulnTimer--;
        setVisible(invulnTimer % 2 == 0);
    } else {
        setVisible(true);
    }

    switch (state) {
    // ========== 飞行 ==========
    case FLYING: {
        damage = 0;

        if (flyTimer == 0) pickNewTarget();

        double dx = targetX - this->x();
        double dy = targetY - this->y();
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 5) {
            double speed = flySpeed;
            if (dist < 100) speed = flySpeed * (dist / 100.0);
            if (speed < 1.5) speed = 1.5;
            vx = (dx / dist) * speed;
            vy = (dy / dist) * speed;
        } else {
            wanderAngle += 0.05;
            vx = std::cos(wanderAngle) * 1.5;
            vy = std::sin(wanderAngle * 1.7) * 2.0;
        }

        animTimer++;
        if (animTimer >= 5) {
            animTimer = 0;
            currentFrame++;
            if (currentFrame >= 7) currentFrame = 0;
            if (currentFrame < allFrames.size())
                setFrame(allFrames[currentFrame], vx < 0);
        }

        // 边界约束
        if (this->x() < 0) { this->setPos(0, this->y()); vx *= -1; }
        if (this->x() > sceneW - frameSize) { this->setPos(sceneW - frameSize, this->y()); vx *= -1; }
        if (this->y() < 0) { this->setPos(this->x(), 0); vy *= -1; }
        if (this->y() > sceneH - frameSize) { this->setPos(this->x(), sceneH - frameSize); vy *= -1; }

        flyTimer++;
        if (flyTimer >= flyDuration) {
            flyTimer = 0;
            state = PREPARING;
            prepareTimer = 0;
            vx = 0; vy = 0;

            // 前摇：显示冲刺帧不动，作为蓄力警告
            double cdx = player->x() - this->x();
            double cdy = player->y() - this->y();
            chargeAngle = std::atan2(cdy, cdx);
            if (allFrames.size() >= 8)
                setFrame(allFrames[7], std::cos(chargeAngle) < 0);
        }
        break;
    }

    // ========== 前摇（蓄力警告） ==========
    case PREPARING: {
        vx = 0; vy = 0;          // 原地停顿
        damage = 0;

        // 持续面向玩家
        double pdx = player->x() - this->x();
        double pdy = player->y() - this->y();
        chargeAngle = std::atan2(pdy, pdx);
        if (allFrames.size() >= 8)
            setFrame(allFrames[7], std::cos(chargeAngle) < 0);

        prepareTimer++;
        if (prepareTimer >= prepareDuration) {
            prepareTimer = 0;
            state = CHARGING;
            chargeTimer = 0;
            chargeSpeed = 0;
            damage = 1;           // 冲刺开始，可造成伤害
        }
        break;
    }

    // ========== 冲刺 ==========
    case CHARGING: {
        chargeSpeed += chargeAccel;
        if (chargeSpeed > maxChargeSpeed) chargeSpeed = maxChargeSpeed;

        vx = std::cos(chargeAngle) * chargeSpeed;
        vy = std::sin(chargeAngle) * chargeSpeed;

        if (allFrames.size() >= 8)
            setFrame(allFrames[7], vx < 0);

        // 边界反弹
        if (this->x() < 0 || this->x() > sceneW - frameSize) {
            vx *= -1; chargeAngle = std::atan2(vy, vx);
        }
        if (this->y() < 0 || this->y() > sceneH - frameSize) {
            vy *= -1; chargeAngle = std::atan2(vy, vx);
        }

        chargeTimer++;
        if (chargeTimer >= chargeDuration) {
            chargeTimer = 0;
            state = COOLDOWN;
            cooldownTimer = 0;
            vx *= 0.3; vy *= 0.3; // 残留惯性
            damage = 0;
        }
        break;
    }

    // ========== 冷却（冲刺后硬直）==========
    case COOLDOWN: {
        damage = 0;

        // 缓慢减速滑行
        vx *= 0.9;
        vy *= 0.9;
        if (std::abs(vx) < 0.3) vx = 0;
        if (std::abs(vy) < 0.3) vy = 0;

        // 冷却时用飞行帧
        animTimer++;
        if (animTimer >= 6) {
            animTimer = 0;
            currentFrame++;
            if (currentFrame >= 7) currentFrame = 0;
            if (currentFrame < allFrames.size())
                setFrame(allFrames[currentFrame], vx < 0);
        }

        cooldownTimer++;
        if (cooldownTimer >= cooldownDuration) {
            cooldownTimer = 0;
            state = FLYING;
            flyTimer = 0;
            vx = 0; vy = 0;
        }
        break;
    }
    }
}
