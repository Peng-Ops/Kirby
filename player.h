#ifndef PLAYER_H
#define PLAYER_H
#include "gameobject.h"
#include <QPixmap>
#include <QVector>
#include <QList>
#include <QPainterPath>
#include "enemy.h"

class Player : public GameObject {
public:
    enum State { IDLE, WALKING, JUMPING, ROLLING, ATTACKING, SWALLOWING, FATTY_IDLE, FATTY_WALKING, SPITTING, DIGESTING };

    // 元素属性（预留用于和场景物品交互：如火融冰、电导水等）
    enum Element {
        ELEM_NONE = 0,
        ELEM_FIRE = 1,
        ELEM_ICE  = 2,
        ELEM_LEAF = 3,
        ELEM_SPARK = 4
    };

    static constexpr double rollSpeed = 8;         // 翻滚时的水平速度

    Player();
    int hp = 5;           // 初始5点生命值
    int invulnTimer = 0;  // 受击无敌时间计时器
    void updateLogic() override;
    void setState(State newState);
    void startSpit();                 // 开始吐出动作
    bool isSpitting = false;          // 是否正在播放吐出动画
    bool triggerSpitStar = false;     // 触发主窗口发射星星的信号旗
    void startRoll();                                // 开始翻滚
    void endRoll();                                  // 结束翻滚
    void startAttack(); // 开始攻击
    void endAttack();   // 结束攻击
    bool isAttacking = false; // 是否正在攻击
    // 找到之前的 isAttacking，在它附近添加：
    bool isLeafSkill = false;   // 是否正在释放叶子技能
    bool isLightningFlying = false;
    bool isLightningDashing = false;
    int lightningDashTimer = 0;
    void startLightningDash();
    void endLightningDash();
    void startLeafSkill();      // 开始释放叶子技能
    void endLeafSkill();        // 结束释放叶子技能
    QPainterPath shape() const override;
    Enemy::CopyAbility swallowedAbility = Enemy::NONE;
    State currentState;
    bool isOnGround = false;
    bool facingRight = true;
    bool isHovering = false;
    bool canDoubleJump = false;
    bool isRolling = false;    // 是否正在翻滚
    int rollTimer = 0;    // 翻滚剩余计时
    bool isFireSprinting = false;     // 是否正在火形态疾跑
    int fireSprintTimer = 0;          // 疾跑剩余时间
    bool isExploding = false;         // 是否正在爆炸
    bool explosionHitEnemy = false;   // 爆炸是否已造成过伤害（防每帧重复）
    int fireSkillCooldownTimer = 0;   // 技能冷却倒计时 (0表示可以使用)
    int leafSkillCooldownTimer = 0;
    void startFireSprint();
    void endFireSprint();
    void startExplosion();
    void startSwallow();
    void endSwallow();
    bool isSwallowing = false;  // 是否正在吞噬
    bool isFatty = false;       // 是否处于 Fatty 状态
    // ====== 新增：蛋糕带来的攻击能力计时器 ======
    int attackPowerTimer = 0; // 剩余攻击能力的帧数
    bool hasAttackPower() const { return attackPowerTimer > 0; }
    int starAttackStock = 0;   // 星星单次攻击储备数
    void startDigest();                               // 开始消化能力
    bool isDigesting = false;                         // 是否正在播放消化变身动画
    bool isIceDefending = false;         // 是否正在冰形态防御
    int iceDefendTimer = 0;              // 防御持续时间计时器 (最多5秒)
    int iceDefendCooldownTimer = 0;      // 防御冷却计时器 (10秒)
    void startIceDefend();               // 开始防御
    void endIceDefend();                 // 结束防御
    bool inWater = false;       // 是否在水中
    int stamina = 300;          // 水下体力值 (例如 300 帧大约 5 秒)
    int maxStamina = 300;       // 最大体力值
    void applySlow(int frames); // 触发减速的函数
    bool isSlowed = false;      // 是否处于减速状态
    Enemy::CopyAbility currentForm = Enemy::NONE;     // 核心：当前卡比持有的形态能力（预留后续扩展空间）
    QList<Enemy::CopyAbility> collectedAbilities;     // 已收集的能力池（最多2个），R键切换

    // 获取当前形态对应的元素属性
    Element currentElement() const {
        switch (currentForm) {
            case Enemy::FIRE:  return ELEM_FIRE;
            case Enemy::ICE:   return ELEM_ICE;
            case Enemy::LEAF:  return ELEM_LEAF;
            case Enemy::SPARK: return ELEM_SPARK;
            default:           return ELEM_NONE;
        }
    }

    int formTimer = 0;  // 特殊形态持续时间（帧数，30秒 = 1800帧）
    int damageFlashTimer = 0;   // 受伤红闪计时器
    int formCancelTimer = 0;  // 长按L取消形态的计时器（60帧=1秒）

    void resetRollAnim() { rollCurrentFrame = 0; rollAnimTimer = 0; }
    void eatCake();     // 吃蛋糕的触发接口
    int cakeAmmo = 0;   // 蛋糕子弹剩余次数（初始为 0）

private:
    int currentFrame = 0;
    int animTimer = 0;
    QVector<QPixmap> idleFrames;
    QVector<QPixmap> walkFrames;
    QVector<QPixmap> jumpFrames;
    QVector<QPixmap> rollFrames;
    QVector<QPixmap> spitFrames;      // 吐出动画帧容器    // 翻滚动画帧
    QVector<QPixmap> attackFrames; // 攻击动画帧
    int attackCurrentFrame = 0;
    int attackAnimTimer = 0;
    int rollCurrentFrame = 0;                        // 翻滚动画当前帧索引
    int rollAnimTimer = 0;                           // 翻滚动画计时
    QVector<QPixmap> swallowFrames;
    QVector<QPixmap> fattyIdleFrames;
    QVector<QPixmap> fattyWalkFrames;
    bool swallowFirstPassDone = false; // 吞噬第一遍是否播放完
    QVector<QPixmap> digestFrames;    // 消化变身动画
    QVector<QPixmap> fireIdleFrames;  // 火形态待机
    QVector<QPixmap> fireWalkFrames;  // 火形态走路
    QVector<QPixmap> fireJumpFrames;  // 火形态飞行跳跃
    QVector<QPixmap> fireRollFrames;
    QVector<QPixmap> fireAttackFrames;  // 火形态攻击动画
    QVector<QPixmap> iceIdleFrames;     // 冰形态待机
    QVector<QPixmap> iceWalkFrames;     // 冰形态走路
    QVector<QPixmap> iceJumpFrames;     // 冰形态飞行跳跃
    QVector<QPixmap> iceRollFrames;
    QVector<QPixmap> iceAttackFrames;   // 冰形态攻击动画
    QVector<QPixmap> leafIdleFrames;    // 叶形态待机
    QVector<QPixmap> leafWalkFrames;    // 叶形态走路
    QVector<QPixmap> leafJumpFrames;    // 叶形态飞行跳跃
    QVector<QPixmap> leafRollFrames;
    QVector<QPixmap> leafAttackFrames;  // 叶形态攻击动画
    QVector<QPixmap> lightningIdleFrames;   // 电形态待机
    QVector<QPixmap> lightningWalkFrames;   // 电形态走路
    QVector<QPixmap> lightningJumpFrames;   // 电形态飞行跳跃
    QVector<QPixmap> lightningRollFrames;
    QVector<QPixmap> lightningAttackFrames; // 电形态攻击动画
    QVector<QPixmap> fireSprintFrames;
    QVector<QPixmap> fireExplodeFrames;
    QVector<QPixmap> iceDefendFrames;    // 冰形态防御动画帧
    int slowTimer = 0;          // 减速剩余帧数计时器
};

#endif // PLAYER_H