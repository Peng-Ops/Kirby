#include "basicenemy.h"
#include <QTransform>
#include <QPainterPath>

MinionEnemy::MinionEnemy(QString spritePath, int frames, double speed, CopyAbility ab) {
    // 1. 把传进来的能力标签存到基类的 ability 变量中
    this->ability = ab;

    // 2. 加载对应的精灵图（自适应帧大小：帧高度=图片高度，帧宽度=高度）
    QPixmap spriteSheet(spritePath);
    int frameSize = spriteSheet.width() / frames; // 水平排列，每帧宽度=总宽度/帧数

    if (!spriteSheet.isNull() && frameSize > 0) {
        int frameH = spriteSheet.height(); // 帧高度=图片高度
        for (int i = 0; i < frames; i++) {
            QPixmap frame = spriteSheet.copy(i * frameSize, 0, frameSize, frameH);
            // 如果原始帧尺寸不是48x48，缩放到48x48以统一碰撞体积
            if (frameSize != 48 || frameH != 48) {
                frame = frame.scaled(48, 48, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            }
            walkFrames.push_back(frame);
        }
        if (!walkFrames.isEmpty()) {
            setPixmap(walkFrames[0]);
        }
    }

    // 3. 设置速度
    walkSpeed = speed;
    vx = -walkSpeed;
}

void MinionEnemy::updateLogic() {
    if (isDead) return;

    // ====== 新增：巡逻计时逻辑 ======
    patrolTimer++;
    if (patrolTimer >= patrolDuration) {
        reverseDirection(); // 时间到了，调用掉头函数
    }

    // 1. 设置速度
    vx = facingRight ? walkSpeed : -walkSpeed;

    // 2. 更新动画 (保持原样)
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
}
void MinionEnemy::reverseDirection() {
    facingRight = !facingRight; // 翻转朝向
    vx = facingRight ? walkSpeed : -walkSpeed; // 立刻同步物理速度，防止悬崖检测用过期方向
    patrolTimer = 0;            // 重点：无论是因为撞墙还是时间到了，掉头后都重新开始计时

    // 立刻翻转精灵图，防止动画更新前的视觉方向与物理方向不一致
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
    // 缩小的碰撞盒，与贴图实际大小匹配，底部对齐
    QPainterPath path;
    path.addRect(4, 4, 40, 44);
    return path;
}
