#ifndef BRAINOFCTHULHU_H
#define BRAINOFCTHULHU_H

#include "bossenemy.h"
#include "player.h"
#include "projectile.h"
#include <QPixmap>
#include <QVector>
#include <QPainterPath>
#include <cmath>
#include <cstdlib>

class BrainOfCthulhu : public BossEnemy {
public:
    enum BossState { FLYING, ATTACKING };

    BrainOfCthulhu(Player* target);
    void updateLogic() override;
    QPainterPath shape() const override;
    void takeDamage(int dmg) override;

    BossState state = FLYING;
    int fullHp = 80;
    bool isPhase2 = false;     // 半血进入二阶段
    int invulnTimer = 0;

    // 主窗口从这里取弹幕加入场景
    QList<Projectile*> pendingProjectiles;

private:
    Player* player;
    QVector<QPixmap> normalFrames;   // 0-3帧: 普通形态
    QVector<QPixmap> phase2Frames;   // 4-7帧: 半血暴走形态
    QVector<QPixmap> attackFrames;   // 弹幕贴图4帧
    int currentFrame = 0;
    int animTimer = 0;
    int frameSize = 96;

    void setFrame(QPixmap img, bool flipLeft);
    void pickNewTarget();

    // 飞行参数
    double flySpeed = 3.0;
    int flyDuration = 150;
    int flyTimer = 0;
    double targetX = 0, targetY = 0;

    // 攻击参数
    int attackDuration = 120;     // 2秒攻击窗口
    int attackTimer = 0;
    int shootInterval = 18;       // 0.3秒发射一波（每秒约3.3波）
    int shootTimer = 0;

    // 场景边界
    static constexpr double sceneW = 5000.0;
    static constexpr double sceneH = 1000.0;
};

#endif // BRAINOFCTHULHU_H
