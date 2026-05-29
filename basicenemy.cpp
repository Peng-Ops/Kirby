#include "basicenemy.h"
#include <QTransform>

MinionEnemy::MinionEnemy(QString spritePath, int frames, double speed, CopyAbility ab) {
    // 1. 把传进来的能力标签存到基类的 ability 变量中
    this->ability = ab;

    // 2. 加载对应的精灵图（自适应帧大小：帧高度=图片高度，帧宽度=高度）
    QPixmap spriteSheet(spritePath);
    int frameSize = spriteSheet.height(); // 自适应高度，支持16px/48px等不同尺寸

    if (!spriteSheet.isNull() && frameSize > 0) {
        for (int i = 0; i < frames; i++) {
            QPixmap frame = spriteSheet.copy(i * frameSize, 0, frameSize, frameSize);
            // 如果原始帧尺寸不是48x48，缩放到48x48以统一碰撞体积
            if (frameSize != 48) {
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
    patrolTimer = 0;            // 重点：无论是因为撞墙还是时间到了，掉头后都重新开始计时
}
