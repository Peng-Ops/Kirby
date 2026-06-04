#include "basicenemy.h"
#include <QTransform>
#include <QPainter>
#include <QPainterPath>
#include <cmath>

MinionEnemy::MinionEnemy(QString spritePath, int frames, double speed, CopyAbility ab, double visualScale) {
    this->ability = ab;
    attackCooldown = 60 + (rand() % 120); // 初始随机冷却，不同步所有敌人

    QPixmap spriteSheet(spritePath);
    int frameW = spriteSheet.width() / frames;
    int frameH = spriteSheet.height();

    if (!spriteSheet.isNull() && frameW > 0) {
        int targetSize = qRound(48 * visualScale);
        for (int i = 0; i < frames; i++) {
            QPixmap frame = spriteSheet.copy(i * frameW, 0, frameW, frameH);
            frame = frame.scaled(targetSize, targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            walkFrames.push_back(frame);
        }
        if (!walkFrames.isEmpty()) {
            setPixmap(walkFrames[0]);
        }
    }

    walkSpeed = speed;
    vx = -walkSpeed;
}

void MinionEnemy::updateLogic() {
    if (isDead) return;

    // 火花冲刺中不执行巡逻逻辑
    if (isDashing) {
        attackStateTimer--;
        animTimer++;
        if (animTimer >= 4) {
            animTimer = 0;
            currentFrame = (currentFrame + 1) % walkFrames.size();
            if (!walkFrames.isEmpty()) {
                QPixmap img = walkFrames[currentFrame];
                if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
                setPixmap(img);
            }
        }
        if (attackStateTimer <= 0) {
            isDashing = false;
            vx = facingRight ? walkSpeed : -walkSpeed;
            damage = 10;
        }
        return;
    }

    if (reverseCooldown > 0) reverseCooldown--;

    patrolTimer++;
    if (patrolTimer >= patrolDuration && reverseCooldown <= 0) {
        reverseDirection();
    }

    vx = facingRight ? walkSpeed : -walkSpeed;

    animTimer++;
    if (animTimer >= 8) {
        animTimer = 0;
        currentFrame++;
        if (currentFrame >= walkFrames.size()) {
            currentFrame = 0;
        }

        if (!walkFrames.isEmpty()) {
            QPixmap img = walkFrames[currentFrame];
            if (facingRight) {
                img = img.transformed(QTransform().scale(-1, 1));
            }
            setPixmap(img);
        }
    }

    // 攻击逻辑
    tryAttack();
}

void MinionEnemy::tryAttack() {
    if (!player || player->hp <= 0) return;
    if (walkFrames.isEmpty()) return;
    if (attackCooldown > 0) {
        attackCooldown--;
        return;
    }

    switch (ability) {
    case FIRE:  doFireAttack();  break;
    case ICE:   doIceAttack();   break;
    case SPARK: doSparkAttack(); break;
    case LEAF:  doLeafAttack();  break;
    default: break;
    }
}

void MinionEnemy::doFireAttack() {
    // 瞄准玩家发射火球
    double dx = player->x() + 24 - this->x() - walkFrames[0].width() / 2.0;
    double dy = player->y() + 24 - this->y() - walkFrames[0].height() / 2.0;
    double dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 20) dist = 20;

    double speed = 3.0;
    double vx = (dx / dist) * speed;
    double vy = (dy / dist) * speed;

    // 创建火球贴图（橙红色圆）
    QPixmap fb(16, 16);
    fb.fill(Qt::transparent);
    QPainter p(&fb);
    p.setBrush(QColor(255, 120, 20));
    p.setPen(Qt::NoPen);
    p.drawEllipse(1, 1, 14, 14);
    p.end();

    Projectile* proj = new Projectile(fb, vx, vy, 200, 1);
    proj->hurtsEnemies = false;
    proj->hurtsPlayer = true;
    proj->setPos(this->x() + walkFrames[0].width() / 2.0 - 8,
                 this->y() + walkFrames[0].height() / 2.0 - 8);
    pendingProjectiles.append(proj);
    attackCooldown = 180; // 3秒冷却
}

void MinionEnemy::doIceAttack() {
    // 冰雾光环：玩家在80px范围内则减速
    if (!player) return;
    double dx = player->x() + 24 - this->x() - walkFrames[0].width() / 2.0;
    double dy = player->y() + 24 - this->y() - walkFrames[0].height() / 2.0;
    double dist = std::sqrt(dx * dx + dy * dy);

    if (dist < 80) {
        player->applySlow(30); // 持续减速（每帧刷新）
    }
    attackCooldown = 10; // 高频检测
}

void MinionEnemy::doSparkAttack() {
    // 向玩家方向冲刺
    if (!player) return;
    double dx = player->x() + 24 - this->x() - walkFrames[0].width() / 2.0;
    double dy = player->y() + 24 - this->y() - walkFrames[0].height() / 2.0;
    double dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 5) dist = 5;

    double dashSpeed = 8.0;
    vx = (dx / dist) * dashSpeed;
    vy = (dy / dist) * dashSpeed;
    facingRight = (dx > 0);  // 冲刺方向与贴图朝向一致
    damage = 2;
    isDashing = true;
    attackStateTimer = 30; // 冲刺30帧
    attackCooldown = 240;  // 4秒冷却
}

void MinionEnemy::doLeafAttack() {
    // 发射叶片弹幕
    if (!player) return;
    double dx = player->x() + 24 - this->x() - walkFrames[0].width() / 2.0;
    double dy = player->y() + 24 - this->y() - walkFrames[0].height() / 2.0;
    double dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 20) dist = 20;

    double speed = 4.0;
    double vx = (dx / dist) * speed;
    double vy = (dy / dist) * speed;

    // 创建叶片贴图（绿色椭圆）
    QPixmap leaf(20, 12);
    leaf.fill(Qt::transparent);
    QPainter p(&leaf);
    p.setBrush(QColor(80, 200, 50));
    p.setPen(Qt::NoPen);
    p.drawEllipse(0, 0, 20, 12);
    p.end();

    Projectile* proj = new Projectile(leaf, vx, vy, 200, 1);
    proj->hurtsEnemies = false;
    proj->hurtsPlayer = true;
    proj->setPos(this->x() + walkFrames[0].width() / 2.0 - 10,
                 this->y() + walkFrames[0].height() / 2.0 - 6);
    pendingProjectiles.append(proj);
    attackCooldown = 200;
}

void MinionEnemy::reverseDirection() {
    facingRight = !facingRight;
    vx = facingRight ? walkSpeed : -walkSpeed;
    patrolTimer = 0;
    reverseCooldown = 15;

    if (!walkFrames.isEmpty() && currentFrame < walkFrames.size()) {
        QPixmap img = walkFrames[currentFrame];
        if (facingRight) {
            img = img.transformed(QTransform().scale(-1, 1));
        }
        setPixmap(img);
    }
}

bool MinionEnemy::canBeSwallowed() {
    return true;
}

QPainterPath MinionEnemy::shape() const {
    QPainterPath path;
    int pw = 48;
    int ph = 48;
    if (!walkFrames.isEmpty()) {
        pw = walkFrames[0].width();
        ph = walkFrames[0].height();
    }
    int cw = 42;
    int ch = 44;
    if (pw < cw + 4) { cw = pw - 4; if (cw < 20) cw = 20; }
    if (ph < ch + 2) { ch = ph - 2; if (ch < 20) ch = 20; }
    path.addRect((pw - cw) / 2, ph - ch, cw, ch);
    return path;
}
