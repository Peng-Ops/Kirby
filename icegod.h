#ifndef ICEGOD_H
#define ICEGOD_H

#include "bossenemy.h"
#include "player.h"
#include "xuehua.h"
#include "projectile.h"
#include <QPixmap>
#include <QVector>
#include <QPainterPath>
#include <cmath>
#include <cstdlib>

class Xuehua; // 前向声明

class IceGod : public BossEnemy {
public:
    enum State { FLYING, SUMMONING };

    IceGod(Player* target);
    void updateLogic() override;
    QPainterPath shape() const override;
    void takeDamage(int dmg) override;

    void summonXuehuas(int count);
    void removeXuehua(Xuehua* x);

    State state = FLYING;
    int fullHp = 60;
    int summonWave = 0;  // 0=初始, 1=2/3血, 2=1/3血

    QList<Xuehua*> xuehuas;
    QList<Xuehua*> pendingXuehuas;   // 新生成的雪花，主窗口从这里取
    QList<Projectile*> pendingProjectiles;

private:
    Player* player;
    QVector<QPixmap> normalFrames;
    int currentFrame = 0;
    int animTimer = 0;
    int frameSize = 96;

    void setFrame(QPixmap img, bool flipLeft);
    void pickNewTarget();

    // 飞行参数
    double flySpeed = 2.5;
    int flyDuration = 200;
    int flyTimer = 0;
    double targetX = 0, targetY = 0;

    // 召唤参数
    int summonDuration = 60;
    int summonTimer = 0;

    // 场景边界
    static constexpr double sceneW = 5000.0;
    static constexpr double sceneH = 1000.0;
};

#endif // ICEGOD_H
