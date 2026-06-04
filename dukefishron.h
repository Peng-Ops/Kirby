#ifndef DUKEFISHRON_H
#define DUKEFISHRON_H

#include "bossenemy.h"
#include "player.h"
#include "tile.h"
#include "projectile.h"
#include <QPixmap>
#include <QVector>
#include <QPainterPath>
#include <cmath>
#include <cstdlib>

class DukeFishron : public BossEnemy {
public:
    // 扩展状态：新增二阶段过渡、二阶段地面、三阶段
    enum BossState { FLYING, PREPARING, CHARGING, COOLDOWN,
                     PHASE_TRANSITION, PHASE_TWO, PHASE_THREE };

    DukeFishron(Player* target);
    void updateLogic() override;
    QPainterPath shape() const override;
    void takeDamage(int dmg) override;

    BossState state = FLYING;
    int invulnTimer = 0;      // 受击闪烁计时
    int fullHp = 300;          // 满血值（血条用）

    // ====== 二/三阶段标记 ======
    bool isPhase2 = false;
    bool isPhase3 = false;
    bool phase2BgTriggered = false;  // 通知主窗口切换背景

    // ====== 二阶段弹幕 ======
    QList<Projectile*> pendingProjectiles;

    // ====== 二阶段水块管理 ======
    struct WaterChangeRecord {
        Tile* tile;
        Tile::TileType originalType;
        QPixmap originalPixmap;
        int timer = 0;          // 从0计数，600帧=10秒后恢复
    };
    QList<Tile*> pendingWaterConversions;          // 待转换的水块
    QList<WaterChangeRecord> activeWaterChanges;   // 正在生效的水块

private:
    Player* player;
    QVector<QPixmap> allFrames;
    int currentFrame = 0;
    int animTimer = 0;
    int frameSize = 96;

    void setFrame(QPixmap img, bool flipLeft);

    // ====== 一阶段飞行参数 ======
    double flySpeed = 3.0;
    int flyDuration = 150;
    int flyTimer = 0;
    double wanderAngle = 0;
    double targetX = 0, targetY = 0;
    void pickNewTarget();

    // ====== 前摇参数 ======
    int prepareDuration = 30;
    int prepareTimer = 0;

    // ====== 冲刺参数 ======
    double chargeAngle = 0;
    double chargeSpeed = 0;
    double chargeAccel = 0.6;
    double maxChargeSpeed = 20.0;
    int chargeDuration = 90;
    int chargeTimer = 0;

    // ====== 冷却参数 ======
    int cooldownDuration = 45;
    int cooldownTimer = 0;

    // ====== 二阶段：变身过渡 ======
    int phaseTransitionTimer = 0;
    double phase2TargetX = 0, phase2TargetY = 0;

    // ====== 二阶段：贴图 ======
    QVector<QPixmap> phase2Frames;     // pigshark2 (8帧)
    QVector<QPixmap> attackFrames;     // pigshark_attack (3帧)

    // ====== 二阶段：弹幕 ======
    int phase2Timer = 0;               // 二阶段总计时 (2400帧=40秒)
    int phase2HoverTimer = 0;          // 悬浮浮动计时
    int phase2ShootTimer = 0;
    int phase2ShootInterval = 30;      // 每30帧发射一波
    int phase2AttackPattern = 0;       // 0=螺旋 1=直线扇型
    double phase2SpiralAngle = 0;      // 螺旋基准角度
    int attackAnimTimer = 0;           // 攻击动画计时
    int attackAnimFrame = 0;

    // ====== 二阶段：水转换 ======
    void convertRandomTilesToWater(int consecutiveCount);

    // ====== 三阶段：速度提升 & 三连冲 ======
    int chargeCount = 0;
    int maxCharges = 1;                // 一阶段=1, 三阶段=3
    double phase3FlySpeed = 4.0;
    double phase3ChargeAccel = 0.8;
    double phase3MaxChargeSpeed = 25.0;

    // 场景边界
    static constexpr double sceneW = 5000.0;
    static constexpr double sceneH = 1200.0;
};

#endif
