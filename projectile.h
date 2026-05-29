#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "gameobject.h"

class Projectile : public GameObject {
public:
    Projectile(bool movingRight);
    // 新增：自定义贴图+任意方向速度（Boss弹幕用）
    Projectile(const QPixmap &pix, double velX, double velY, int lifetime = 60, int dmg = 1);
    void updateLogic() override;

    int damage = 1;      // 攻击力
    int lifeTime = 60;   // 存活时间（帧数）
    bool hurtsPlayer = false;  // Boss弹幕打玩家
    bool hurtsEnemies = true;  // 玩家弹幕打敌人
    bool hasGravity = false;   // 是否受重力影响（冰锥等）

    // 动画支持（Boss弹幕用）
    QVector<QPixmap> animFrames;
    int animTimer = 0;
    int animFrameIdx = 0;
};

#endif // PROJECTILE_H