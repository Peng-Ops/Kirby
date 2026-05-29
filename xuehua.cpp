#include "xuehua.h"
#include "icegod.h"
#include "projectile.h"
#include <QPainter>
#include <cmath>

Xuehua::Xuehua(IceGod* owner, Player* target)
    : iceGod(owner), player(target)
{
    hp = 3;
    damage = 1;
    ignoresTiles = true; // 飞行单位，无视地形

    // 加载雪花贴图（竖6帧）
    QPixmap sheet(":/tu/xuehuaguai.png");
    if (!sheet.isNull()) {
        int fw = sheet.width();   // 58
        int fh = sheet.height() / 6; // 58
        for (int i = 0; i < 6; i++) {
            QPixmap frame = sheet.copy(0, i * fh, fw, fh);
            normalFrames.push_back(frame);
        }
    }

    // 兜底
    if (normalFrames.isEmpty()) {
        QPixmap fallback(58, 58);
        fallback.fill(Qt::transparent);
        QPainter p(&fallback);
        p.setBrush(QColor(180, 220, 255));
        p.setPen(Qt::NoPen);
        p.drawEllipse(4, 4, 50, 50);
        p.end();
        normalFrames.push_back(fallback);
    }

    setPixmap(normalFrames[0]);

    // 预加载冰锥弹幕贴图（竖3帧），缩放到 16×24
    QPixmap atkSheet(":/tu/icegod_attack.png");
    if (!atkSheet.isNull()) {
        int fw = atkSheet.width();    // 24
        int fh = atkSheet.height() / 3; // 74
        for (int i = 0; i < 3; i++) {
            QPixmap frame = atkSheet.copy(0, i * fh, fw, fh);
            frame = frame.scaled(16, 24, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            atkFrames.push_back(frame);
        }
    }
    if (atkFrames.isEmpty()) {
        QPixmap fallback(16, 24);
        fallback.fill(Qt::transparent);
        QPainter p(&fallback);
        p.setBrush(QColor(100, 200, 255));
        p.setPen(Qt::NoPen);
        p.drawRect(2, 0, 12, 24);
        p.end();
        atkFrames.push_back(fallback);
    }
}

QPainterPath Xuehua::shape() const {
    QPainterPath path;
    path.addRect(0, 0, 58, 58);
    return path;
}

void Xuehua::takeDamage(int dmg) {
    // 火形态伤害翻倍
    int actualDmg = dmg;
    if (player && player->currentForm == Enemy::FIRE) {
        actualDmg *= 2;
    }
    Enemy::takeDamage(actualDmg);
    if (isDead && iceGod) {
        iceGod->removeXuehua(this);
    }
}

void Xuehua::updateLogic() {
    if (isDead || !iceGod) return;

    // 轨道环绕
    orbitAngle += angularSpeed;
    if (orbitAngle > 2 * M_PI) orbitAngle -= 2 * M_PI;

    double cx = iceGod->x() + 48; // IceGod中心X (frameSize/2)
    double cy = iceGod->y() + 48; // IceGod中心Y
    double nx = cx + std::cos(orbitAngle) * orbitRadius - 29; // 29 = 58/2
    double ny = cy + std::sin(orbitAngle) * orbitRadius - 29;
    setPos(nx, ny);

    // 动画
    animTimer++;
    if (animTimer >= 6) {
        animTimer = 0;
        currentFrame = (currentFrame + 1) % normalFrames.size();
        setPixmap(normalFrames[currentFrame]);
    }

    // 射击：冰锥垂直落下
    if (player) {
        shootTimer++;
        if (shootTimer >= shootInterval) {
            shootTimer = 0;

            // 在雪花正下方生成冰锥，以随机水平偏移增加变化
            double offsetX = (rand() % 60) - 30;
            Projectile* p = new Projectile(
                atkFrames[0],   // 贴图
                0,              // vx = 0，垂直落下
                1.0,            // 初始微小的向下速度
                150,            // 寿命2.5秒（足够落到地面）
                1               // 伤害
            );
            p->animFrames = atkFrames;
            p->hurtsEnemies = false;
            p->hurtsPlayer = true;
            p->hasGravity = true;  // 触发自由落体
            p->setPos(this->x() + 29 + offsetX - 8, this->y() + 58);

            iceGod->pendingProjectiles.append(p);
        }
    }
}
