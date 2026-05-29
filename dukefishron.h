#ifndef DUKEFISHRON_H
#define DUKEFISHRON_H

#include "bossenemy.h"
#include "player.h"
#include <QPixmap>
#include <QVector>
#include <QPainterPath>
#include <cmath>
#include <cstdlib>

class DukeFishron : public BossEnemy {
public:
    enum BossState { FLYING, PREPARING, CHARGING, COOLDOWN };

    DukeFishron(Player* target);
    void updateLogic() override;
    QPainterPath shape() const override;
    void takeDamage(int dmg) override;

    BossState state = FLYING;
    int invulnTimer = 0;      // 受击闪烁计时
    int fullHp = 50;          // 满血值（血条用）

private:
    Player* player;
    QVector<QPixmap> allFrames;
    int currentFrame = 0;
    int animTimer = 0;
    int frameSize = 96;

    void setFrame(QPixmap img, bool flipLeft);

    // 飞行参数
    double flySpeed = 3.0;
    int flyDuration = 150;
    int flyTimer = 0;
    double wanderAngle = 0;
    double targetX = 0, targetY = 0;
    void pickNewTarget();

    // 前摇参数
    int prepareDuration = 30;   // 0.5秒蓄力
    int prepareTimer = 0;

    // 冲刺参数
    double chargeAngle = 0;
    double chargeSpeed = 0;
    double chargeAccel = 0.6;
    double maxChargeSpeed = 20.0;
    int chargeDuration = 90;
    int chargeTimer = 0;

    // 冷却参数
    int cooldownDuration = 45;
    int cooldownTimer = 0;

    // 场景边界
    static constexpr double sceneW = 5000.0;
    static constexpr double sceneH = 1000.0;
};

#endif
