#include "icegod.h"
#include <QTransform>
#include <QPainter>
#include <algorithm>

IceGod::IceGod(Player* target)
    : player(target)
{
    hp = fullHp;
    damage = 0;

    // 加载Boss本体贴图（竖6帧），缩放至 frameSize×frameSize
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
        p.setBrush(QColor(100, 180, 255));
        p.setPen(Qt::NoPen);
        p.drawEllipse(4, 4, frameSize - 8, frameSize - 8);
        p.end();
        normalFrames.push_back(fallback);
    }

    setPixmap(normalFrames[0]);
    setOffset((frameSize - normalFrames[0].width()) / 2.0,
              frameSize - normalFrames[0].height());

    // 加载冰锥弹幕贴图（icegod_attack.png 竖3帧），缩放到 24×36
    QPixmap atkSheet(":/tu/icegod_attack.png");
    if (!atkSheet.isNull()) {
        int fw = atkSheet.width();         // 24
        int fh = atkSheet.height() / 3;    // 74
        for (int i = 0; i < 3; i++) {
            QPixmap frame = atkSheet.copy(0, i * fh, fw, fh);
            frame = frame.scaled(24, 36, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            projectileFrames.push_back(frame);
        }
    }
    if (projectileFrames.isEmpty()) {
        QPixmap fallback(24, 36);
        fallback.fill(Qt::transparent);
        QPainter p(&fallback);
        p.setBrush(QColor(80, 180, 255));
        p.setPen(Qt::NoPen);
        p.drawEllipse(2, 2, 20, 32);
        p.end();
        projectileFrames.push_back(fallback);
    }
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
    // 碰撞箱约为贴图中心区域
    path.addRect(16, 16, frameSize - 32, frameSize - 32);
    return path;
}

void IceGod::takeDamage(int dmg) {
    if (state == DYING) return;

    int actualDmg = dmg;
    // 火形态伤害翻倍
    if (player && player->currentForm == Enemy::FIRE) {
        actualDmg *= 2;
    }
    Enemy::takeDamage(actualDmg);
    if (isDead) {
        isDead = false;
        hp = 1;
        isDying = true;
        state = DYING;
        deathTimer = 0;
        vx = 0; vy = 0;
        damage = 0;
        setVisible(true);
        // 清理所有雪花（标记死亡并隐藏）
        for (Xuehua* x : xuehuas) {
            x->isDead = true;
            x->setVisible(false);
        }
        xuehuas.clear();
        pendingXuehuas.clear();
        pendingProjectiles.clear();
    }

    // 检查召唤波次
    if (!isDying) {
        int newWave = -1;
        if (summonWave == 0 && hp <= fullHp * 2 / 3) {
            newWave = 1;
        } else if (summonWave <= 1 && hp <= fullHp * 1 / 3) {
            newWave = 2;
        }
        if (newWave > summonWave) {
            summonWave = newWave;
            int count = (newWave == 1) ? 4 : 6;
            summonXuehuas(count);
        }
    }
}

void IceGod::summonXuehuas(int count) {
    double angleStep = 2 * M_PI / count;
    for (int i = 0; i < count; i++) {
        Xuehua* x = new Xuehua(this, player);
        x->orbitAngle = angleStep * i;
        // 低血量时轨道半径更小，更密集
        if (summonWave >= 2) {
            x->orbitRadius = 70.0;
            x->angularSpeed = 0.04;
        }
        xuehuas.append(x);
        pendingXuehuas.append(x);
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
    // 限制Y在屏幕可见范围 [350, 900]
    if (targetY < 350) targetY = 350;
    if (targetY > 900) targetY = 900;
}

// ====== 冰锥圆形扩散 ======
void IceGod::doIceSpread() {
    vx = 0;
    vy = 0;
    attackTimer++;
    if (attackTimer % 25 == 0) {
        int count = 8 + (summonWave >= 1 ? 4 : 0) + (summonWave >= 2 ? 4 : 0);
        double baseAngle = attackTimer * 0.03; // 每轮旋转，形成螺旋效果
        double speed = 3.0 + summonWave * 0.5;
        for (int i = 0; i < count; i++) {
            double angle = 2 * M_PI * i / count + baseAngle;
            Projectile* p = new Projectile(
                projectileFrames[0],
                std::cos(angle) * speed,
                std::sin(angle) * speed,
                99999,     // 无限距离
                20
            );
            p->hurtsEnemies = false;
            p->hurtsPlayer = true;
            p->ignoresWalls = true;
            p->causesSlow = true;
            p->setPos(this->x() + frameSize / 2.0 - 12,
                      this->y() + frameSize / 2.0 - 18);
            pendingProjectiles.append(p);
        }
        attackPhase++;
    }
    if (attackPhase >= 3) {
        attackPhase = 0;
        attackTimer = 0;
        state = FLYING;
        flyTimer = 0;
    }
}

// ====== 瞄准冰束 ======
void IceGod::doIceBeam() {
    if (!player) { state = FLYING; flyTimer = 0; return; }
    vx = 0;
    vy = 0;
    attackTimer++;
    if (attackTimer % 12 == 0 && attackPhase < 6) {
        double dx = player->x() - this->x();
        double dy = player->y() - this->y();
        double baseAngle = std::atan2(dy, dx);
        int beamCount = 3 + summonWave; // 3~5条
        double spread = 0.12;           // 散射角度
        double speed = 5.0 + summonWave * 0.5;
        for (int i = 0; i < beamCount; i++) {
            double angle = baseAngle + (i - (beamCount - 1) / 2.0) * spread;
            Projectile* p = new Projectile(
                projectileFrames[0],
                std::cos(angle) * speed,
                std::sin(angle) * speed,
                99999,     // 无限距离
                20
            );
            p->hurtsEnemies = false;
            p->hurtsPlayer = true;
            p->ignoresWalls = true;
            p->causesSlow = true;
            p->setPos(this->x() + frameSize / 2.0 - 12,
                      this->y() + frameSize / 2.0 - 18);
            pendingProjectiles.append(p);
        }
        attackPhase++;
    }
    if (attackPhase >= 6 || attackTimer > 120) {
        attackPhase = 0;
        attackTimer = 0;
        state = FLYING;
        flyTimer = 0;
    }
}

// ====== 冰锥雨 ======
void IceGod::doIceRain() {
    if (!player) { state = FLYING; flyTimer = 0; return; }
    vx = 0;
    vy = 0;
    attackTimer++;
    if (attackTimer % 8 == 0) {
        // 在玩家周围随机位置生成冰锥
        double spread = 300.0 + summonWave * 50;
        double x = player->x() + (rand() % (int)spread) - spread / 2.0;
        x = std::max(0.0, std::min(x, sceneW - 50.0));
        Projectile* p = new Projectile(
            projectileFrames[0],
            (rand() % 100 - 50) * 0.02,    // 轻微水平偏移
            3.0 + (rand() % 100) * 0.03,    // 下落速度
            99999,                           // 无限距离
            15
        );
        p->hasGravity = true;
        p->hurtsEnemies = false;
        p->hurtsPlayer = true;
        p->ignoresWalls = true;
        p->causesSlow = true;
        p->setPos(x, this->y() - 50);
        pendingProjectiles.append(p);
        attackPhase++;
    }
    // 持续射出一定数量后结束
    int maxPhase = 12 + summonWave * 4;
    if (attackPhase >= maxPhase || attackTimer > 180) {
        attackPhase = 0;
        attackTimer = 0;
        state = FLYING;
        flyTimer = 0;
    }
}

void IceGod::updateLogic() {
    if (isDead || !player) return;

    // 清理已死亡的雪花
    for (int i = xuehuas.size() - 1; i >= 0; i--) {
        if (xuehuas[i]->isDead) {
            xuehuas.removeAt(i);
        }
    }

    // ====== 死亡动画 ======
    if (state == DYING) {
        deathTimer++;
        if (deathTimer < 90) {
            int interval = 8 - deathTimer / 11;
            if (interval < 1) interval = 1;
            setVisible((deathTimer / interval) % 2 == 0);
        } else if (deathTimer < 120) {
            setVisible(true);
        }
        return;
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

        // 动画
        animTimer++;
        if (animTimer >= 12) {
            animTimer = 0;
            currentFrame = (currentFrame + 1) % normalFrames.size();
            if (currentFrame < normalFrames.size())
                setFrame(normalFrames[currentFrame], vx < 0);
        }

        // 边界——限制在屏幕可见范围
        if (this->x() < 0) { this->setPos(0, this->y()); vx *= -1; }
        if (this->x() > sceneW - frameSize) { this->setPos(sceneW - frameSize, this->y()); vx *= -1; }
        if (this->y() < 200) { this->setPos(this->x(), 200); vy *= -1; }
        if (this->y() > 1050) { this->setPos(this->x(), 1050); vy *= -1; }

        flyTimer++;
        if (flyTimer >= flyDuration) {
            flyTimer = 0;
            vx = 0; vy = 0;

            // 随机选择下一个行动：召唤 / 三种攻击
            int r = rand() % 100;
            if (r < 25) {
                state = SUMMONING;
                summonTimer = 0;
            } else if (r < 50) {
                state = ICE_SPREAD;
                attackTimer = 0;
                attackPhase = 0;
            } else if (r < 75) {
                state = ICE_BEAM;
                attackTimer = 0;
                attackPhase = 0;
            } else {
                state = ICE_RAIN;
                attackTimer = 0;
                attackPhase = 0;
            }
        }
        break;
    }

    case SUMMONING: {
        vx = 0; vy = 0;
        damage = 0;

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
            // 召唤结束时额外生成雪花（HP越低生成越多）
            int extraCount = 0;
            if (summonWave >= 1) extraCount = 1;
            if (summonWave >= 2) extraCount = 2;
            if (extraCount > 0) {
                summonXuehuas(extraCount);
            }
            state = FLYING;
            flyTimer = 0;
        }
        break;
    }

    case ICE_SPREAD:
        damage = 0;
        doIceSpread();
        break;

    case ICE_BEAM:
        damage = 0;
        doIceBeam();
        break;

    case ICE_RAIN:
        damage = 0;
        doIceRain();
        break;
    }
}
