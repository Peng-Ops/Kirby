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
    enum State {
        FLYING,       // 飞行移动
        SUMMONING,    // 召唤雪花
        ICE_SPREAD,   // 圆形冰锥扩散
        ICE_BEAM,     // 瞄准冰束
        ICE_RAIN,     // 冰锥雨
        DYING         // 死亡动画
    };

    IceGod(Player* target);
    void updateLogic() override;
    QPainterPath shape() const override;
    void takeDamage(int dmg) override;

    void summonXuehuas(int count);
    void removeXuehua(Xuehua* x);

    State state = FLYING;
    int fullHp = 600;
    int summonWave = 0;  // 0=初始, 1=2/3血, 2=1/3血
    bool isDying = false;
    int deathTimer = 0;  // 死亡动画计时器

    QList<Xuehua*> xuehuas;
    QList<Xuehua*> pendingXuehuas;   // 新生成的雪花，主窗口从这里取
    QList<Projectile*> pendingProjectiles;

    static constexpr int frameSize = 192;  // 公开给Xuehua等使用

private:
    Player* player;
    QVector<QPixmap> normalFrames;
    QVector<QPixmap> projectileFrames;  // 冰锥弹幕贴图
    int currentFrame = 0;
    int animTimer = 0;

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

    // 攻击参数
    int attackPhase = 0;
    int attackTimer = 0;

    // 场景边界
    static constexpr double sceneW = 5000.0;
    static constexpr double sceneH = 1200.0;

    // 攻击模式
    void doIceSpread();
    void doIceBeam();
    void doIceRain();
};

#endif // ICEGOD_H
