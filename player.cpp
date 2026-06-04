#include "player.h"
#include <QTransform>
#include <QGraphicsColorizeEffect>

Player::Player() {
    int frameSize = 48;

    // 翻滚
    QPixmap rollSheet(":/tu/kirby_fangun.png");
    int rollCount = rollSheet.width() / frameSize;
    for (int i = 0; i < rollCount; i++) {
        rollFrames.push_back(rollSheet.copy(i * frameSize, 0, frameSize, frameSize));
    }

    // 待机
    QPixmap daijiSheet(":/tu/kirby_daiji.png");
    int daijiCount = daijiSheet.width() / frameSize;
    for (int i = 0; i < daijiCount; i++) {
        idleFrames.push_back(daijiSheet.copy(i * frameSize, 0, frameSize, frameSize));
    }

    // 走路
    QPixmap walkSheet(":/tu/kirby_walk.png");
    int walkCount = walkSheet.width() / frameSize;
    for (int i = 0; i < walkCount; i++) {
        walkFrames.push_back(walkSheet.copy(i * frameSize, 0, frameSize, frameSize));
    }

    // 飞行/跳跃
    QPixmap flySheet(":/tu/kirby_fly.png");
    int flyCount = flySheet.width() / frameSize;
    for (int i = 0; i < flyCount; i++) {
        jumpFrames.push_back(flySheet.copy(i * frameSize, 0, frameSize, frameSize));
    }

    // 攻击（新增）
    QPixmap attackSheet(":/tu/normal-attack.png");
    // 根据你提供的图，看起来是7帧，我们根据宽度均分
    int attackCount = 7;
    int attackFrameWidth = attackSheet.width() / attackCount;
    for (int i = 0; i < attackCount; i++) {
        attackFrames.push_back(attackSheet.copy(i * attackFrameWidth, 0, attackFrameWidth, attackSheet.height()));
    }

    if (!idleFrames.isEmpty()) {
        setPixmap(idleFrames[0]);
    }
    // 吞噬动画加载 (72x72, 共5帧)
    QPixmap swallowSheet(":/tu/kirby_swallow.png");
    for (int i = 0; i < 1; i++) {
        swallowFrames.push_back(swallowSheet.copy(i * 72, 0, 72, 72));
    }
    for (int i = 0; i < 5; i++) {
        swallowFrames.push_back(swallowSheet.copy(i * 72, 0, 72, 72));
    }

    // Fatty 待机动画加载 (自适应高度切片)
    QPixmap fattyDaijiSheet(":/tu/fatty_daiji.png");
    if (!fattyDaijiSheet.isNull()) {
        int fh = fattyDaijiSheet.height();
        int count = fattyDaijiSheet.width() / fh;
        for (int i = 0; i < count; i++) {
            fattyIdleFrames.push_back(fattyDaijiSheet.copy(i * fh, 0, fh, fh));
        }
    }

    // Fatty 走路动画加载 (自适应高度切片)
    QPixmap fattyWalkSheet(":/tu/fatty_walk.png");
    if (!fattyWalkSheet.isNull()) {
        int fh = fattyWalkSheet.height();
        int count = fattyWalkSheet.width() / fh;
        for (int i = 0; i < count; i++) {
            fattyWalkFrames.push_back(fattyWalkSheet.copy(i * fh, 0, fh, fh));
        }
    }

    // 吐出动画加载 (自适应高度切片，自动识别网格尺寸)
    QPixmap tuchuSheet(":/tu/kirby_tuchu.png");
    if (!tuchuSheet.isNull()) {
        int fh = tuchuSheet.height();
        int count = tuchuSheet.width() / fh;
        for (int i = 0; i < count; i++) {
            spitFrames.push_back(tuchuSheet.copy(i * fh, 0, fh, fh));
        }
    }
    // 消化/变身动画加载 (自适应高度)
    QPixmap digestSheet(":/tu/kirby_digest.png");
    if (!digestSheet.isNull()) {
        int fh = digestSheet.height();
        int count = digestSheet.width() / fh;
        for (int i = 0; i < count; i++) {
            digestFrames.push_back(digestSheet.copy(i * fh, 0, fh, fh));
        }
    }

    // 火形态：待机动画加载 (自适应高度)
    QPixmap fireDaijiSheet(":/tu/fire_daiji.png");
    if (!fireDaijiSheet.isNull()) {
        int fh = fireDaijiSheet.height();
        int count = fireDaijiSheet.width() / fh;
        for (int i = 0; i < count; i++) {
            fireIdleFrames.push_back(fireDaijiSheet.copy(i * fh, 0, fh, fh));
        }
    }

    // 火形态：走路动画加载 (自适应高度)
    QPixmap fireWalkSheet(":/tu/fire_walk.png");
    if (!fireWalkSheet.isNull()) {
        int fh = fireWalkSheet.height();
        int count = fireWalkSheet.width() / fh;
        for (int i = 0; i < count; i++) {
            fireWalkFrames.push_back(fireWalkSheet.copy(i * fh, 0, fh, fh));
        }
    }

    // 火形态：飞行/跳跃动画加载 (自适应高度)
    QPixmap fireFlySheet(":/tu/fire_fly.png");
    if (!fireFlySheet.isNull()) {
        int fh = fireFlySheet.height();
        int count = fireFlySheet.width() / fh;
        for (int i = 0; i < count; i++) {
            fireJumpFrames.push_back(fireFlySheet.copy(i * fh, 0, fh, fh));
        }
    }
    // 火形态：翻滚动画加载 (自适应高度)
    QPixmap fireRollSheet(":/tu/fire_fangun.png");
    if (!fireRollSheet.isNull()) {
        int fh = fireRollSheet.height();
        int count = fireRollSheet.width() / fh;
        for (int i = 0; i < count; i++) {
            fireRollFrames.push_back(fireRollSheet.copy(i * fh, 0, fh, fh));
        }
    }
    // ====== 火形态新技能素材加载 ======
    QPixmap sprintSheet(":/tu/fire_speedrun.png");
    if (!sprintSheet.isNull()) {
        int count = 4; // 假设是一排4帧的动画
        int fw = sprintSheet.width() / count;
        int fh = sprintSheet.height();
        for (int i = 0; i < count; i++) fireSprintFrames.push_back(sprintSheet.copy(i * fw, 0, fw, fh));
    }

    QPixmap explodeSheet(":/tu/fire_baozha.png");
    if (!explodeSheet.isNull()) {
        int count = 4;
        int fw = explodeSheet.width() / count;
        int fh = explodeSheet.height();
        for (int i = 0; i < count; i++) fireExplodeFrames.push_back(explodeSheet.copy(i * fw, 0, fw, fh));
    }


    // ====== 修复这里：强行清空被旧逻辑错误加载的整张大图 ======
    leafAttackFrames.clear();

    // 重新按水平 3 帧切割新的动作
    QPixmap leafAttackSheet(":/tu/leaf_attack.png");
    if (!leafAttackSheet.isNull()) {
        int count = 3;
        int fw = leafAttackSheet.width() / count;
        int fh = leafAttackSheet.height();
        for (int i = 0; i < count; i++) {
            leafAttackFrames.push_back(leafAttackSheet.copy(i * fw, 0, fw, fh));
        }
    }

    QPixmap iceDefSheet(":/tu/ice_defend.png");
    if (!iceDefSheet.isNull()) {
        int count = 4;
        int fw = iceDefSheet.width() / count;
        int fh = iceDefSheet.height();
        for (int i = 0; i < count; i++) {
            iceDefendFrames.push_back(iceDefSheet.copy(i * fw, 0, fw, fh));
        }
    }
    // ====== 冰形态素材加载 ======
    auto loadFormSprites = [](QString prefix, QVector<QPixmap>& idle, QVector<QPixmap>& walk,
                               QVector<QPixmap>& jump, QVector<QPixmap>& roll, QVector<QPixmap>& attack) {
        // 待机
        QPixmap idleSheet(":/tu/" + prefix + "_daiji.png");
        if (!idleSheet.isNull()) {
            int fh = idleSheet.height();
            int count = idleSheet.width() / fh;
            for (int i = 0; i < count; i++)
                idle.push_back(idleSheet.copy(i * fh, 0, fh, fh));
        }
        // 走路
        QPixmap walkSheet(":/tu/" + prefix + "_walk.png");
        if (!walkSheet.isNull()) {
            int fh = walkSheet.height();
            int count = walkSheet.width() / fh;
            for (int i = 0; i < count; i++)
                walk.push_back(walkSheet.copy(i * fh, 0, fh, fh));
        }
        // 飞行/跳跃
        QPixmap flySheet(":/tu/" + prefix + "_fly.png");
        if (!flySheet.isNull()) {
            int fh = flySheet.height();
            int count = flySheet.width() / fh;
            for (int i = 0; i < count; i++)
                jump.push_back(flySheet.copy(i * fh, 0, fh, fh));
        }
        // 翻滚
        QPixmap rollSheet(":/tu/" + prefix + "_fangun.png");
        if (!rollSheet.isNull()) {
            int fh = rollSheet.height();
            int count = rollSheet.width() / fh;
            for (int i = 0; i < count; i++)
                roll.push_back(rollSheet.copy(i * fh, 0, fh, fh));
        }
        // 攻击
        QPixmap attackSheet(":/tu/" + prefix + "_attack.png");
        if (!attackSheet.isNull()) {
            int fh = attackSheet.height();
            int count = attackSheet.width() / fh;
            for (int i = 0; i < count; i++)
                attack.push_back(attackSheet.copy(i * fh, 0, fh, fh));
        }
    };



    loadFormSprites("ice", iceIdleFrames, iceWalkFrames, iceJumpFrames, iceRollFrames, iceAttackFrames);
    loadFormSprites("leaf", leafIdleFrames, leafWalkFrames, leafJumpFrames, leafRollFrames, leafAttackFrames);
    loadFormSprites("lightning", lightningIdleFrames, lightningWalkFrames, lightningJumpFrames, lightningRollFrames, lightningAttackFrames);
}

void Player::setState(State newState) {
    if (currentState != newState) {
        currentState = newState;
        currentFrame = 0;
        animTimer = 0;
    }
}
void Player::startFireSprint() {
    if (isFireSprinting || isExploding) return;
    isFireSprinting = true;
    fireSprintTimer = 240;          // 冲刺持续 60 帧 (约 1 秒)
    fireSkillCooldownTimer = 0;  // 技能冷却 180 帧 (约 3 秒)
    currentFrame = 0;
    animTimer = 0;
}

void Player::endFireSprint() {
    isFireSprinting = false;
    setState(isOnGround ? IDLE : JUMPING);
}

void Player::startExplosion() {
    isFireSprinting = false;
    isExploding = true;
    currentFrame = 0;
    animTimer = 0;
}
void Player::startRoll() {
    if (isRolling || isExploding || isAttacking || isLeafSkill || isSwallowing || isSpitting || isDigesting || isFireSprinting) return;
    // 电形态飞行时按K变为冲刺（不翻滚）
    if (isLightningFlying) {
        startLightningDash();
        return;
    }
    // 根据形态选择翻滚帧
    QVector<QPixmap>* activeRollFrames = &rollFrames;
    if (currentForm == Enemy::FIRE && !fireRollFrames.isEmpty()) activeRollFrames = &fireRollFrames;
    else if (currentForm == Enemy::ICE && !iceRollFrames.isEmpty()) activeRollFrames = &iceRollFrames;
    else if (currentForm == Enemy::LEAF && !leafRollFrames.isEmpty()) activeRollFrames = &leafRollFrames;
    else if (currentForm == Enemy::SPARK && !lightningRollFrames.isEmpty()) activeRollFrames = &lightningRollFrames;

    int frameCount = activeRollFrames->size();
    if (frameCount == 0) return;

    isRolling = true;
    setState(ROLLING);
    rollTimer = frameCount * 2;
    rollCurrentFrame = 0;
    rollAnimTimer = 0;

    QPixmap img = (*activeRollFrames)[0];
    if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
    setOffset((48 - img.width()) / 2.0, 48 - img.height());
    setPixmap(img);
}

void Player::endRoll() {
    isRolling = false;
    // 恢复到地面待机或空中状态（交由下次updateLogic判断）
    setState(isOnGround ? IDLE : JUMPING);
}

void Player::startLightningDash() {
    isLightningDashing = true;
    lightningDashTimer = 15;     // 冲刺持续 15 帧 (~0.25秒)
    // 冲刺时用一个快速动画帧
    if (!lightningJumpFrames.isEmpty()) {
        QPixmap img = lightningJumpFrames[0];
        if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
        setOffset((48 - img.width()) / 2.0, 48 - img.height());
        setPixmap(img);
    }
}

void Player::endLightningDash() {
    isLightningDashing = false;
    lightningDashTimer = 0;
    // 恢复电形态飞行
}

void Player::applySlow(int frames) {
    slowTimer = frames;
    isSlowed = true;
}

void Player::updateLogic() {
    if (currentForm != Enemy::NONE && !isDigesting) {
        if (formTimer > 0) {
            formTimer--;
            if (formTimer <= 0) {
                currentForm = Enemy::NONE; // ⏳ 时间到，强制变回普通形态

                // 安全拦截：强制关闭可能正在运行的形态专属技能
                isIceDefending = false;
                isFireSprinting = false;
                isLightningDashing = false;
                isLeafSkill = false;
            }
        }
    }
    // 受伤红闪效果（优先级最高，覆盖其他颜色效果）
    if (damageFlashTimer > 0) {
        damageFlashTimer--;
        QGraphicsColorizeEffect *redEffect = new QGraphicsColorizeEffect();
        redEffect->setColor(QColor(255, 60, 40));
        this->setGraphicsEffect(redEffect);
    } else if (slowTimer > 0) {
        slowTimer--;
        isSlowed = (slowTimer > 0);
        QGraphicsColorizeEffect *iceEffect = new QGraphicsColorizeEffect();
        iceEffect->setColor(QColor(100, 200, 255)); // 冰蓝色
        this->setGraphicsEffect(iceEffect);
    } else {
        isSlowed = false;
        this->setGraphicsEffect(nullptr);
    }
    // 1. 技能冷却倒计时
    if (fireSkillCooldownTimer > 0) fireSkillCooldownTimer--;
    if (leafSkillCooldownTimer > 0) leafSkillCooldownTimer--;
    if (iceDefendCooldownTimer > 0) iceDefendCooldownTimer--;
    if (isIceDefending) {
        vx = 0; // 防御期间锁死水平物理移动
        iceDefendTimer--;

        attackAnimTimer++;
        if (attackAnimTimer >= 6) { // 每 6 帧切换一次动画
            attackAnimTimer = 0;
            attackCurrentFrame++;

            // 核心逻辑：如果播到了第 4 帧（越界），强制拉回第 2 帧（即第三张图），实现最后两帧无限循环
            if (attackCurrentFrame >= iceDefendFrames.size()) {
                attackCurrentFrame = 2;
            }
        }

        if (!iceDefendFrames.isEmpty()) {
            QPixmap img = iceDefendFrames[attackCurrentFrame];
            if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
            setOffset((48 - img.width()) / 2.0, 48 - img.height());
            setPixmap(img);
        }

        // 5秒时间耗尽，强制解除防御并进入冷却
        if (iceDefendTimer <= 0) {
            endIceDefend();
        }
        return;
    }
    // 2. 受击无敌时间计时与闪烁效果
    if (invulnTimer > 0) {
        invulnTimer--;
        if (invulnTimer % 4 < 2) setVisible(false);
        else setVisible(true);
    } else {
        setVisible(true);
    }

    // ====== 电形态飞行冲刺（按K触发，向前突进一段） ======
    if (isLightningDashing) {
        vx = facingRight ? 15.0 : -15.0;
        lightningDashTimer--;
        if (lightningDashTimer <= 0) {
            endLightningDash();
        }
        // 冲刺时播放飞行帧
        animTimer++;
        if (animTimer >= 3) {
            animTimer = 0;
            currentFrame++;
            if (currentFrame >= lightningJumpFrames.size()) currentFrame = 0;
            if (!lightningJumpFrames.isEmpty()) {
                QPixmap img = lightningJumpFrames[currentFrame];
                if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
                setOffset((48 - img.width()) / 2.0, 48 - img.height());
                setPixmap(img);
            }
        }
        return;
    }

    // ====== 新技能：电形态飞行拦截 ======
    if (isLightningFlying) {
        animTimer++;
        if (animTimer >= 3) {
            animTimer = 0;
            currentFrame++;
            if (currentFrame >= lightningJumpFrames.size()) currentFrame = 0;

            if (!lightningJumpFrames.isEmpty()) {
                QPixmap img = lightningJumpFrames[currentFrame];
                if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
                setOffset((48 - img.width()) / 2.0, 48 - img.height());
                setPixmap(img);
            }
        }
        return;
    }

    // ====== 新技能：爆炸状态拦截 ======
    if (isExploding) {
        vx = 0; vy = 0;
        animTimer++;
        if (animTimer >= 4) {
            animTimer = 0;
            currentFrame++;
            if (currentFrame >= fireExplodeFrames.size() ) {
                isExploding = false;
                setState(isOnGround ? IDLE : JUMPING);
                return;
            }
            if (!fireExplodeFrames.isEmpty()) {
                QPixmap img = fireExplodeFrames[currentFrame % fireExplodeFrames.size()];
                if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
                setOffset((48 - img.width()) / 2.0, 48 - img.height());
                setPixmap(img);
            }
        }
        return;
    }

    // ====== 新技能：火系疾跑状态拦截 ======
    if (isFireSprinting) {
        vx = facingRight ? 12.0 : -12.0;
        fireSprintTimer--;

        if (fireSprintTimer <= 0) {
            endFireSprint();
            return;
        }

        animTimer++;
        if (animTimer >= 3) {
            animTimer = 0;
            rollCurrentFrame++;
            if (rollCurrentFrame >= fireSprintFrames.size()) rollCurrentFrame = 0;

            if (!fireSprintFrames.isEmpty()) {
                QPixmap img = fireSprintFrames[rollCurrentFrame];
                if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
                setOffset((48 - img.width()) / 2.0, 48 - img.height());
                setPixmap(img);
            }
        }
        return;
    }

    // ====== 新技能：叶子发羽毛拦截 ======
    if (isLeafSkill) {
        if (isOnGround) vx = 0;
        attackAnimTimer++;
        if (attackAnimTimer >= 3) {
            attackAnimTimer = 0;
            attackCurrentFrame++;
            if (attackCurrentFrame >= leafAttackFrames.size()) {
                endLeafSkill();
                return;
            }
            QPixmap img = leafAttackFrames[attackCurrentFrame];
            if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
            setOffset((48 - img.width()) / 2.0, 48 - img.height());
            setPixmap(img);
        }
        return;
    }

    // ====== 纯净版：普通攻击状态拦截 ======
    if (isAttacking) {
        if (isOnGround) vx = 0;
        attackAnimTimer++;
        if (attackAnimTimer >= 3) {
            attackAnimTimer = 0;
            attackCurrentFrame++;
            if (attackCurrentFrame >= attackFrames.size()) {
                endAttack();
                return;
            }
            QPixmap img = attackFrames[attackCurrentFrame];
            if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
            setOffset((48 - img.width()) / 2.0, 48 - img.height());
            setPixmap(img);
        }
        return;
    }

    // ====== 核心找回：变身消化状态拦截 ======
    if (isDigesting) {
        if (isOnGround) vx = 0;
        animTimer++;
        if (animTimer >= 5) {
            animTimer = 0;
            currentFrame++;

            // 消化动画播放完毕
            if (currentFrame >= digestFrames.size()) {
                isDigesting = false;
                currentForm = swallowedAbility; // 核心：正式继承肚子里的怪物能力！
                // 将能力加入收集池（最多保留2个，不重复）
                if (swallowedAbility != Enemy::NONE && !collectedAbilities.contains(swallowedAbility)) {
                    if (collectedAbilities.size() >= 2) {
                        collectedAbilities.removeFirst(); // 移除最旧的
                    }
                    collectedAbilities.append(swallowedAbility);
                }
                swallowedAbility = Enemy::NONE; // 消化干净，清空胃部
                formTimer = 1200;
                setState(isOnGround ? IDLE : JUMPING);
                return;
            }

            if (!digestFrames.isEmpty()) {
                QPixmap img = digestFrames[currentFrame];
                if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
                setOffset((48 - img.width()) / 2.0, 48 - img.height());
                setPixmap(img);
            }
        }
        return;
    }

    // ====== 核心找回：吐出状态拦截 ======
    if (isSpitting) {
        if (isOnGround) vx = 0;
        animTimer++;
        if (animTimer >= 3) {
            animTimer = 0;
            currentFrame++;
            if (currentFrame == 1) triggerSpitStar = true;
            if (currentFrame >= spitFrames.size()) {
                isSpitting = false;
                if (cakeAmmo <= 0) {
                    isFatty = false;
                }
                setState(isOnGround ? IDLE : JUMPING);
                return;
            }
            if (!spitFrames.isEmpty()) {
                QPixmap img = spitFrames[currentFrame];
                if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
                setOffset((48 - img.width()) / 2.0, 48 - img.height());
                setPixmap(img);
            }
        }
        return;
    }

    // ====== 核心找回：吞噬状态拦截 ======
    if (isSwallowing) {
        if (isOnGround) vx = 0;
        animTimer++;
        if (animTimer >= 5) {
            animTimer = 0;
            if (!swallowFirstPassDone) {
                currentFrame++;
                if (currentFrame >= 5) {
                    swallowFirstPassDone = true;
                    currentFrame = 3;
                }
            } else {
                currentFrame++;
                if (currentFrame > 4) currentFrame = 3;
            }
            if (!swallowFrames.isEmpty()) {
                QPixmap img = swallowFrames[currentFrame];
                if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
                setOffset((48 - img.width()) / 2.0, 48 - img.height());
                setPixmap(img);
            }
        }
        return;
    }

    // ====== 核心找回：Fatty(变胖)状态拦截 ======
    if (isFatty) {
        State targetState = (qAbs(vx) < 0.1) ? FATTY_IDLE : FATTY_WALKING;
        if (targetState != currentState) {
            currentState = targetState;
            currentFrame = 0;
            animTimer = 0;
        }
        animTimer++;
        if (animTimer >= 6) {
            animTimer = 0;
            currentFrame++;
            QPixmap img;
            if (currentState == FATTY_IDLE) {
                if (!fattyIdleFrames.isEmpty()) {
                    if (currentFrame >= fattyIdleFrames.size()) currentFrame = 0;
                    img = fattyIdleFrames[currentFrame];
                }
            } else {
                if (!fattyWalkFrames.isEmpty()) {
                    if (currentFrame >= fattyWalkFrames.size()) currentFrame = 0;
                    img = fattyWalkFrames[currentFrame];
                }
            }
            if (!facingRight && !img.isNull())
                img = img.transformed(QTransform().scale(-1, 1));
            if (!img.isNull()) {
                setOffset((48 - img.width()) / 2.0, 48 - img.height());
                setPixmap(img);
            }
        }
        return;
    }

    // ====== 翻滚中 ======
    if (isRolling) {
        rollAnimTimer++;
        if (rollAnimTimer >= 2) {
            rollAnimTimer = 0;
            rollCurrentFrame++;

            QVector<QPixmap>* activeRollFrames = &rollFrames;
            if (currentForm == Enemy::FIRE && !fireRollFrames.isEmpty()) activeRollFrames = &fireRollFrames;
            else if (currentForm == Enemy::ICE && !iceRollFrames.isEmpty()) activeRollFrames = &iceRollFrames;
            else if (currentForm == Enemy::LEAF && !leafRollFrames.isEmpty()) activeRollFrames = &leafRollFrames;
            else if (currentForm == Enemy::SPARK && !lightningRollFrames.isEmpty()) activeRollFrames = &lightningRollFrames;

            int frameCount = activeRollFrames->size();
            if (rollCurrentFrame >= frameCount) {
                rollCurrentFrame = frameCount - 1;
            }

            QPixmap img = (*activeRollFrames)[rollCurrentFrame];
            if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
            setOffset((48 - img.width()) / 2.0, 48 - img.height());
            setPixmap(img);
        }
        return;
    }

    // ====== 常规物理运动状态切换 ======
    State targetState;
    if (!isOnGround) {
        targetState = JUMPING;
    } else {
        if (qAbs(vx) < 0.1) targetState = IDLE;
        else targetState = WALKING;
    }

    if (targetState != currentState) {
        currentState = targetState;
        currentFrame = 0;
    }

    animTimer++;
    if (animTimer >= 6) {
        animTimer = 0;
        currentFrame++;

        QPixmap currentImage;
        switch (currentState) {
        case JUMPING: {
            QVector<QPixmap>* frames = &jumpFrames;
            bool isTransformed = (currentForm != Enemy::NONE);

            if (currentForm == Enemy::FIRE && !fireJumpFrames.isEmpty()) frames = &fireJumpFrames;
            else if (currentForm == Enemy::ICE && !iceJumpFrames.isEmpty()) frames = &iceJumpFrames;
            else if (currentForm == Enemy::LEAF && !leafJumpFrames.isEmpty()) frames = &leafJumpFrames;
            else if (currentForm == Enemy::SPARK && !lightningJumpFrames.isEmpty()) frames = &lightningJumpFrames;

            if (!frames->isEmpty()) {
                if (!isTransformed) {
                    if (currentFrame <= 6) currentFrame = 6;
                    if (currentFrame >= 15) currentFrame = 6;
                    int safeFrame = currentFrame;
                    if (safeFrame >= frames->size()) safeFrame = frames->size() - 1;
                    currentImage = (*frames)[safeFrame];
                } else {
                    int slowFrame = (currentFrame / 3) % frames->size();
                    currentImage = (*frames)[slowFrame];
                }
            }
            break;
        }
        case WALKING: {
            QVector<QPixmap>* frames = &walkFrames;
            if (currentForm == Enemy::FIRE && !fireWalkFrames.isEmpty()) frames = &fireWalkFrames;
            else if (currentForm == Enemy::ICE && !iceWalkFrames.isEmpty()) frames = &iceWalkFrames;
            else if (currentForm == Enemy::LEAF && !leafWalkFrames.isEmpty()) frames = &leafWalkFrames;
            else if (currentForm == Enemy::SPARK && !lightningWalkFrames.isEmpty()) frames = &lightningWalkFrames;

            if (!frames->isEmpty()) {
                if (currentFrame >= frames->size()) currentFrame = 0;
                currentImage = (*frames)[currentFrame];
            }
            break;
        }
        case IDLE:
        default: {
            QVector<QPixmap>* frames = &idleFrames;
            if (currentForm == Enemy::FIRE && !fireIdleFrames.isEmpty()) frames = &fireIdleFrames;
            else if (currentForm == Enemy::ICE && !iceIdleFrames.isEmpty()) frames = &iceIdleFrames;
            else if (currentForm == Enemy::LEAF && !leafIdleFrames.isEmpty()) frames = &leafIdleFrames;
            else if (currentForm == Enemy::SPARK && !lightningIdleFrames.isEmpty()) frames = &lightningIdleFrames;

            if (!frames->isEmpty()) {
                if (currentFrame >= frames->size()) currentFrame = 0;
                currentImage = (*frames)[currentFrame];
            }
            break;
        }
        }

        if (!facingRight && !currentImage.isNull())
            currentImage = currentImage.transformed(QTransform().scale(-1, 1));

        if (!currentImage.isNull()) {
            setOffset((48 - currentImage.width()) / 2.0, 48 - currentImage.height());
            setPixmap(currentImage);
        }
    }
    if (currentForm == Enemy::SPARK && !isLightningDashing) {
        vx *= 1.5; // 基础左右走路/飞行速度加快 50%
    }
}
QPainterPath Player::shape() const {
    QPainterPath path;
    path.addRect(4, 4, 40, 44);
    return path;
}
// ====== 1. 纯净版的普通攻击 ======
void Player::startAttack() {
    // 只有普通形态(NONE)可以发动普通攻击
    if (currentForm != Enemy::NONE || attackFrames.isEmpty()) return;
    if (isAttacking || isRolling || isLeafSkill) return;

    isAttacking = true;
    setState(ATTACKING);
    attackCurrentFrame = 0;
    attackAnimTimer = 0;

    QPixmap img = attackFrames[0];
    if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
    setOffset((48 - img.width()) / 2.0, 48 - img.height());
    setPixmap(img);
}
void Player::endAttack() {
    isAttacking = false;
    setState(isOnGround ? IDLE : JUMPING);
}
// ====== 2. 全新的叶子技能专属动作 ======
void Player::startLeafSkill() {
    if (isLeafSkill || isRolling || leafAttackFrames.isEmpty()) return;

    isLeafSkill = true;
    // 借用 ATTACKING 状态来打断常规动画的播放
    setState(ATTACKING);
    attackCurrentFrame = 0;
    attackAnimTimer = 0;

    QPixmap img = leafAttackFrames[0];
    if (!facingRight) img = img.transformed(QTransform().scale(-1, 1));
    setOffset((48 - img.width()) / 2.0, 48 - img.height());
    setPixmap(img);
}
void Player::endLeafSkill() {
    isLeafSkill = false;
    setState(isOnGround ? IDLE : JUMPING);
}
void Player::startSwallow() {
    // ====== 修改：增加 currentForm != Enemy::NONE 拦截 ======
    // 只有在初始状态（NONE）时，才能张嘴吞噬。拥有任何能力时绝对禁止吞噬。
    if (isSwallowing || isRolling || isAttacking || isFatty || isDigesting || isSpitting || !isOnGround || currentForm != Enemy::NONE) return;
    isSwallowing = true;
    swallowFirstPassDone = false;
    setState(SWALLOWING);
    currentFrame = 0;
    animTimer = 0;
}
void Player::endSwallow() {
    if (!isSwallowing) return;
    isSwallowing = false;
    setState(isFatty ? FATTY_IDLE : (isOnGround ? IDLE : JUMPING));
}
// 当 MainWindow 检测到卡比碰撞到蛋糕时，调用这个函数
void Player::eatCake() {
    if (currentForm != Enemy::NONE) return; // 如果已经是特殊形态，不能吃蛋糕
    isFatty = true;
    cakeAmmo = 2; // 赋予 2 次发射机会
}
void Player::startSpit() {
    if (isSpitting || cakeAmmo <= 0) return;
    isSpitting = true;
    cakeAmmo--;
    triggerSpitStar = false;  // 重置子弹发射旗帜
    setState(SPITTING);
    currentFrame = 0;
    animTimer = 0;
}
void Player::startDigest() {
    if (isDigesting || !isFatty) return;
    isDigesting = true;
    isFatty = false; // 瞬间解除肥胖视觉，由消化动作接管画面
    setState(DIGESTING);
    currentFrame = 0;
    animTimer = 0;
}

void Player::startIceDefend() {
    if (isIceDefending || iceDefendCooldownTimer > 0 || iceDefendFrames.isEmpty()) return;
    isIceDefending = true;
    iceDefendTimer = 300; // 5秒 * 60帧 = 300帧
    setState(ATTACKING);  // 借用攻击状态打断常规步行动画
    attackCurrentFrame = 0;
    attackAnimTimer = 0;
}

void Player::endIceDefend() {
    if (!isIceDefending) return;
    isIceDefending = false;
    iceDefendCooldownTimer = 600; // 10秒 * 60帧 = 600帧冷却
    setState(isOnGround ? IDLE : JUMPING);
}