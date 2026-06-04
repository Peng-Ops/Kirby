#ifndef BASICENEMY_H
#define BASICENEMY_H

#include "enemy.h"
#include "projectile.h"
#include "player.h"
#include <QPixmap>
#include <QVector>

class Player;

// 小怪类：所有Dude类敌人（火、冰、叶、电）均归入此类
class MinionEnemy : public Enemy {
public:
    MinionEnemy(QString spritePath, int frames, double speed, CopyAbility ab, double visualScale = 1.0);
    void updateLogic() override;
    void reverseDirection() override;
    bool canBeSwallowed() override;
    bool isFacingRight() const { return facingRight; }
    void setPatrolDuration(int frames) { patrolDuration = frames; }
    void setPlayer(Player* p) { player = p; }
    QPainterPath shape() const override;

    // 攻击系统：每帧生成的弹幕暂存于此，由MainWindow取出加入场景
    QList<Projectile*> pendingProjectiles;

private:
    int currentFrame = 0;
    int animTimer = 0;
    QVector<QPixmap> walkFrames;

    bool facingRight = false;
    double walkSpeed = 1.5;
    int patrolTimer = 0;
    int patrolDuration = 120;
    int reverseCooldown = 0;

    // 攻击系统
    Player* player = nullptr;
    int attackCooldown = 0;   // 攻击冷却帧数
    int attackStateTimer = 0; // 当前攻击状态持续时间（用于火花冲刺等）
    bool isDashing = false;   // 火花冲刺中

    void tryAttack();
    void doFireAttack();
    void doIceAttack();
    void doSparkAttack();
    void doLeafAttack();
};

#endif // BASICENEMY_H
