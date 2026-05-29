#ifndef XUEHUA_H
#define XUEHUA_H

#include "enemy.h"
#include "player.h"
#include <QPixmap>
#include <QVector>
#include <QPainterPath>

class IceGod; // 前向声明

class Xuehua : public Enemy {
public:
    Xuehua(IceGod* owner, Player* target);
    void updateLogic() override;
    QPainterPath shape() const override;
    void takeDamage(int dmg) override;
    bool canBeSwallowed() override { return false; }

    IceGod* iceGod;
    Player* player;
    double orbitAngle = 0;       // 当前轨道角度（弧度）
    double orbitRadius = 100.0;  // 轨道半径
    double angularSpeed = 0.03;  // 角速度（弧度/帧）

private:
    QVector<QPixmap> normalFrames;
    QVector<QPixmap> atkFrames;  // 预加载冰锥弹幕贴图
    int currentFrame = 0;
    int animTimer = 0;
    int shootTimer = 0;
    int shootInterval = 60;      // 每60帧(1秒)射击一次
};

#endif // XUEHUA_H
