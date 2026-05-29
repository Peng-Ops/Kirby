#ifndef BASICENEMY_H
#define BASICENEMY_H

#include "enemy.h"
#include <QPixmap>
#include <QVector>

// 小怪类：所有Dude类敌人（火、冰、叶、电）均归入此类
class MinionEnemy : public Enemy {
public:
    // 构造函数新增参数：图片路径，总帧数，移动速度，以及它携带的能力
    MinionEnemy(QString spritePath, int frames, double speed, CopyAbility ab);
    void updateLogic() override;
    void reverseDirection() override;

private:
    int currentFrame = 0;
    int animTimer = 0;
    QVector<QPixmap> walkFrames;

    bool facingRight = false;
    double walkSpeed = 1.5;
    int patrolTimer = 0;      // 当前已经走了多久
    int patrolDuration = 120; // 设定走多久掉头（120帧约等于走2秒钟）
};

#endif // BASICENEMY_H
