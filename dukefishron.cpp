#include "dukefishron.h"
#include <QTransform>
#include <QGraphicsScene>

DukeFishron::DukeFishron(Player* target)
    : player(target)
{
    hp = fullHp;
    damage = 0;

    // 加载一阶贴图 pig_shark.png (竖8帧)
    QPixmap spriteSheet(":/tu/pig_shark.png");
    if (!spriteSheet.isNull()) {
        int fw = spriteSheet.width();
        int fh = spriteSheet.height() / 8;
        for (int i = 0; i < 8; i++) {
            QPixmap frame = spriteSheet.copy(0, i * fh, fw, fh);
            frame = frame.scaled(frameSize, frameSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            allFrames.push_back(frame);
        }
        if (!allFrames.isEmpty()) {
            QPixmap img = allFrames[0];
            setOffset((frameSize - img.width()) / 2.0, frameSize - img.height());
            setPixmap(img);
        }
    }

    // 加载二阶贴图 pigshark2.png (竖8帧, 每帧202x162)
    QPixmap phase2Sheet(":/tu/pigshark2.png");
    if (!phase2Sheet.isNull()) {
        int fw = phase2Sheet.width();          // 202
        int fh = phase2Sheet.height() / 8;    // 162
        for (int i = 0; i < 8; i++) {
            QPixmap frame = phase2Sheet.copy(0, i * fh, fw, fh);
            frame = frame.scaled(frameSize, frameSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            phase2Frames.push_back(frame);
        }
    }

    // 加载攻击弹幕贴图 pigshark_attack.png (横3帧, 每帧48x48)
    QPixmap atkSheet(":/tu/pigshark_attack.png");
    if (!atkSheet.isNull()) {
        int fw = atkSheet.width() / 3;   // 48
        int fh = atkSheet.height();      // 48
        for (int i = 0; i < 3; i++) {
            QPixmap frame = atkSheet.copy(i * fw, 0, fw, fh);
            frame = frame.scaled(32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            attackFrames.push_back(frame);
        }
    }
}

void DukeFishron::setFrame(QPixmap img, bool flipLeft) {
    if (flipLeft) {
        img = img.transformed(QTransform().scale(-1, 1));
    }
    setOffset((frameSize - img.width()) / 2.0, frameSize - img.height());
    setPixmap(img);
}

QPainterPath DukeFishron::shape() const {
    QPainterPath path;
    path.addRect(8, 8, 80, 80);
    return path;
}

void DukeFishron::takeDamage(int dmg) {
    // 二阶段无敌
    if (state == PHASE_TWO) return;

    Enemy::takeDamage(dmg);
    if (!isDead && dmg > 0) {
        invulnTimer = 8;
    }

    // 半血触发二阶段
    if (!isPhase2 && hp <= fullHp / 2 && !isDead) {
        isPhase2 = true;
        state = PHASE_TRANSITION;
        phaseTransitionTimer = 0;
        vx = 0;
        vy = 0;
        damage = 0;
        setVisible(true);   // 确保Boss可见，否则activeBoss检测失败会停止刷怪和血条

        // 飞向屏幕中上方
        phase2TargetX = sceneW / 2.0 - frameSize / 2.0;
        phase2TargetY = -30.0;

        // 切换到二阶贴图
        if (!phase2Frames.isEmpty()) {
            setFrame(phase2Frames[0], false);
        }
    }
}

void DukeFishron::pickNewTarget() {
    if (!player) return;
    double offsetX = (rand() % 300 + 200) * (rand() % 2 == 0 ? 1 : -1);
    targetX = player->x() + offsetX;
    // 限制Y在屏幕可见范围 [350, 900]
    targetY = (double)(350 + rand() % 550);
    if (targetX < 0) targetX = 0;
    if (targetX > sceneW - frameSize) targetX = sceneW - frameSize;
    if (targetY < 350) targetY = 350;
    if (targetY > 900) targetY = 900;
}

void DukeFishron::convertRandomTilesToWater(int consecutiveCount) {
    if (!scene()) return;

    // 收集场景中所有可转换的固体地面Tile
    QList<QGraphicsItem*> items = scene()->items();
    QList<Tile*> allCandidates;
    for (QGraphicsItem* item : items) {
        Tile* tile = dynamic_cast<Tile*>(item);
        if (!tile) continue;
        Tile::TileType type = tile->tileType();
        if (type == Tile::Grass || type == Tile::Dirt ||
            type == Tile::IceBlock || type == Tile::RubbleBlock) {
            // 排除已在pending或active中的
            bool alreadyChanging = false;
            for (Tile* pt : pendingWaterConversions) {
                if (pt == tile) { alreadyChanging = true; break; }
            }
            if (!alreadyChanging) {
                for (auto& wc : activeWaterChanges) {
                    if (wc.tile == tile) { alreadyChanging = true; break; }
                }
            }
            if (!alreadyChanging) {
                allCandidates.append(tile);
            }
        }
    }

    // 只取地表方块（上方没有其他固体方块的）
    QList<Tile*> solidTiles;
    qreal tileH = 48.0; // 标准方块高度
    for (Tile* tile : allCandidates) {
        bool hasTileAbove = false;
        qreal aboveY = tile->y() - tileH;
        for (Tile* other : allCandidates) {
            if (other == tile) continue;
            if (std::abs(other->x() - tile->x()) < 4.0 &&
                std::abs(other->y() - aboveY) < 4.0) {
                hasTileAbove = true;
                break;
            }
        }
        if (!hasTileAbove) {
            solidTiles.append(tile);
        }
    }

    if (solidTiles.isEmpty()) return;

    int startIdx = rand() % solidTiles.size();
    Tile* startTile = solidTiles[startIdx];
    qreal startX = startTile->x();
    qreal tileW = startTile->sceneBoundingRect().width();
    if (tileW <= 0) tileW = 48;

    pendingWaterConversions.append(startTile);

    for (int i = 1; i < consecutiveCount; i++) {
        qreal expectedX = startX + i * tileW;
        for (Tile* t : solidTiles) {
            if (std::abs(t->x() - expectedX) < 4.0) {
                pendingWaterConversions.append(t);
                break;
            }
        }
    }
}

void DukeFishron::convertRandomTilesToWater(int consecutiveCount) {
    if (!scene()) return;

    // 收集场景中所有可转换的固体地面Tile
    QList<QGraphicsItem*> items = scene()->items();
    QList<Tile*> allCandidates;
    for (QGraphicsItem* item : items) {
        Tile* tile = dynamic_cast<Tile*>(item);
        if (!tile) continue;
        Tile::TileType type = tile->tileType();
        if (type == Tile::Grass || type == Tile::Dirt ||
            type == Tile::IceBlock || type == Tile::RubbleBlock) {
            // 排除已在pending或active中的
            bool alreadyChanging = false;
            for (Tile* pt : pendingWaterConversions) {
                if (pt == tile) { alreadyChanging = true; break; }
            }
            if (!alreadyChanging) {
                for (auto& wc : activeWaterChanges) {
                    if (wc.tile == tile) { alreadyChanging = true; break; }
                }
            }
            if (!alreadyChanging) {
                allCandidates.append(tile);
            }
        }
    }

    // 只取地表方块（上方没有其他固体方块的）
    QList<Tile*> solidTiles;
    qreal tileH = 48.0; // 标准方块高度
    for (Tile* tile : allCandidates) {
        bool hasTileAbove = false;
        qreal aboveY = tile->y() - tileH;
        for (Tile* other : allCandidates) {
            if (other == tile) continue;
            if (std::abs(other->x() - tile->x()) < 4.0 &&
                std::abs(other->y() - aboveY) < 4.0) {
                hasTileAbove = true;
                break;
            }
        }
        if (!hasTileAbove) {
            solidTiles.append(tile);
        }
    }

    if (solidTiles.isEmpty()) return;

    int startIdx = rand() % solidTiles.size();
    Tile* startTile = solidTiles[startIdx];
    qreal startX = startTile->x();
    qreal tileW = startTile->sceneBoundingRect().width();
    if (tileW <= 0) tileW = 48;

    pendingWaterConversions.append(startTile);

    for (int i = 1; i < consecutiveCount; i++) {
        qreal expectedX = startX + i * tileW;
        for (Tile* t : solidTiles) {
            if (std::abs(t->x() - expectedX) < 4.0) {
                pendingWaterConversions.append(t);
                break;
            }
        }
    }
}

void DukeFishron::updateLogic() {
    if (isDead || !player) return;

    // ====== PHASE_TRANSITION: 飞向屏幕中上方（2秒无敌变身） ======
    if (state == PHASE_TRANSITION) {
        damage = 0;
        double dx = phase2TargetX - this->x();
        double dy = phase2TargetY - this->y();
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 5) {
            double speed = 5.0;
            vx = (dx / dist) * speed;
            vy = (dy / dist) * speed;
        } else {
            vx = 0;
            vy = 0;
        }

        // 飞行中播放phase2动画
        animTimer++;
        if (animTimer >= 5) {
            animTimer = 0;
            currentFrame = (currentFrame + 1) % phase2Frames.size();
            if (currentFrame < phase2Frames.size())
                setFrame(phase2Frames[currentFrame], vx < 0);
        }

        phaseTransitionTimer++;
        if (phaseTransitionTimer >= 120) {  // 2秒后进入二阶段
            phaseTransitionTimer = 0;
            state = PHASE_TWO;
            phase2Timer = 0;
            phase2ShootTimer = 0;
            phase2SpiralAngle = 0;
            phase2AttackPattern = 0;
            attackAnimTimer = 0;
            attackAnimFrame = 0;

            // 悬浮在半空中
            this->setPos(this->x(), 150.0);

            // 通知主窗口切换背景
            phase2BgTriggered = true;
        }
        setPos(x() + vx, y() + vy);
        return;
    }

    // ====== PHASE_TWO: 地面无敌阶段（40秒） ======
    if (state == PHASE_TWO) {
        damage = 0;

        // 在半空中悬浮，跟踪玩家水平位置
        phase2HoverTimer++;
        double hoverSpeed = 2.5;
        double targetHoverX = player->x() - frameSize / 2.0;
        if (targetHoverX < 0) targetHoverX = 0;
        if (targetHoverX > sceneW - frameSize) targetHoverX = sceneW - frameSize;
        double dx = targetHoverX - this->x();
        vx = dx * 0.02;
        if (vx > hoverSpeed) vx = hoverSpeed;
        if (vx < -hoverSpeed) vx = -hoverSpeed;

        // Y轴轻微上下浮动
        double hoverY = 600.0 + std::sin(phase2HoverTimer * 0.02) * 60.0;
        double dy = hoverY - this->y();
        vy = dy * 0.05;

        // 身位动画
        animTimer++;
        if (animTimer >= 10) {
            animTimer = 0;
            currentFrame = (currentFrame + 1) % phase2Frames.size();
            if (currentFrame < phase2Frames.size())
                setFrame(phase2Frames[currentFrame], player->x() < this->x());
        }

        // 攻击动画（弹幕贴图循环）
        attackAnimTimer++;
        if (attackAnimTimer >= 8) {
            attackAnimTimer = 0;
            attackAnimFrame = (attackAnimFrame + 1) % attackFrames.size();
        }

        // === 水环射击 ===
        phase2ShootTimer++;
        if (phase2ShootTimer >= phase2ShootInterval) {
            phase2ShootTimer = 0;

            if (phase2AttackPattern == 0) {
                // 螺旋模式：8方向旋转
                int ringCount = 8;
                double speed = 4.0;
                for (int i = 0; i < ringCount; i++) {
                    double angle = 2 * M_PI * i / ringCount + phase2SpiralAngle;
                    Projectile* p = new Projectile(
                        attackFrames[attackAnimFrame],
                        std::cos(angle) * speed,
                        std::sin(angle) * speed,
                        99999, 10
                    );
                    p->hurtsEnemies = false;
                    p->hurtsPlayer = true;
                    p->ignoresWalls = true;
                    p->setPos(this->x() + frameSize / 2.0 - 16,
                              this->y() + frameSize / 2.0 - 16);
                    pendingProjectiles.append(p);
                }
                phase2SpiralAngle += 0.2;
            } else {
                // 直线扇型模式：瞄准玩家
                double dx = player->x() - this->x();
                double dy = player->y() - this->y();
                double baseAngle = std::atan2(dy, dx);
                int beamCount = 5;
                double spread = 0.15;
                double speed = 5.0;
                for (int i = 0; i < beamCount; i++) {
                    double angle = baseAngle + (i - (beamCount - 1) / 2.0) * spread;
                    Projectile* p = new Projectile(
                        attackFrames[attackAnimFrame],
                        std::cos(angle) * speed,
                        std::sin(angle) * speed,
                        99999, 10
                    );
                    p->hurtsEnemies = false;
                    p->hurtsPlayer = true;
                    p->ignoresWalls = true;
                    p->setPos(this->x() + frameSize / 2.0 - 16,
                              this->y() + frameSize / 2.0 - 16);
                    pendingProjectiles.append(p);
                }
            }
            phase2AttackPattern = (phase2AttackPattern + 1) % 2;  // 交替
        }

        // === 地面变水（每3秒触发一次） ===
        if (phase2Timer % 180 == 0 && phase2Timer > 0) {
            convertRandomTilesToWater(2);
        }

        phase2Timer++;

        // 40秒（2400帧）后进入三阶段
        if (phase2Timer >= 2400) {
            state = PHASE_THREE;
            isPhase3 = true;
            phase2Timer = 0;
            damage = 20;  // 恢复可造成伤害

            // 切回一阶贴图
            if (!allFrames.isEmpty()) {
                setFrame(allFrames[0], false);
            }

            // 恢复一阶状态机
            state = FLYING;
            flyTimer = 0;
            vx = 0;
            vy = 0;
            chargeCount = 0;
            maxCharges = 3;  // 三连冲
        }
        return;
    }

    // ====== 受伤闪烁 ======
    if (invulnTimer > 0) {
        invulnTimer--;
        setVisible(invulnTimer % 2 == 0);
    } else {
        setVisible(true);
    }

    // ====== 一阶段/三阶段共用状态机 ======
    double currentFlySpeed = isPhase3 ? phase3FlySpeed : flySpeed;
    double currentChargeAccel = isPhase3 ? phase3ChargeAccel : chargeAccel;
    double currentMaxChargeSpeed = isPhase3 ? phase3MaxChargeSpeed : maxChargeSpeed;

    switch (state) {
    // ========== 飞行 ==========
    case FLYING: {
        damage = 0;

        if (flyTimer == 0) pickNewTarget();

        double dx = targetX - this->x();
        double dy = targetY - this->y();
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 5) {
            double speed = currentFlySpeed;
            if (dist < 100) speed = currentFlySpeed * (dist / 100.0);
            if (speed < 1.5) speed = 1.5;
            vx = (dx / dist) * speed;
            vy = (dy / dist) * speed;
        } else {
            wanderAngle += 0.05;
            vx = std::cos(wanderAngle) * 1.5;
            vy = std::sin(wanderAngle * 1.7) * 2.0;
        }

        animTimer++;
        if (animTimer >= 5) {
            animTimer = 0;
            currentFrame++;
            if (currentFrame >= 7) currentFrame = 0;
            if (currentFrame < allFrames.size())
                setFrame(allFrames[currentFrame], vx < 0);
        }

        // 边界约束——限制在屏幕可见范围
        if (this->x() < 0) { this->setPos(0, this->y()); vx *= -1; }
        if (this->x() > sceneW - frameSize) { this->setPos(sceneW - frameSize, this->y()); vx *= -1; }
        if (this->y() < 200) { this->setPos(this->x(), 200); vy *= -1; }
        if (this->y() > 1050) { this->setPos(this->x(), 1050); vy *= -1; }

        flyTimer++;
        if (flyTimer >= flyDuration) {
            flyTimer = 0;
            state = PREPARING;
            prepareTimer = 0;
            vx = 0; vy = 0;

            double cdx = player->x() - this->x();
            double cdy = player->y() - this->y();
            chargeAngle = std::atan2(cdy, cdx);
            if (allFrames.size() >= 8)
                setFrame(allFrames[7], std::cos(chargeAngle) < 0);
        }
        break;
    }

    // ========== 前摇 ==========
    case PREPARING: {
        vx = 0; vy = 0;
        damage = 0;

        double pdx = player->x() - this->x();
        double pdy = player->y() - this->y();
        chargeAngle = std::atan2(pdy, pdx);
        if (allFrames.size() >= 8)
            setFrame(allFrames[7], std::cos(chargeAngle) < 0);

        prepareTimer++;
        if (prepareTimer >= prepareDuration) {
            prepareTimer = 0;
            state = CHARGING;
            chargeTimer = 0;
            chargeSpeed = 0;
            damage = 20;

            chargeCount++;  // 记录本次连冲计数
        }
        break;
    }

    // ========== 冲刺 ==========
    case CHARGING: {
        chargeSpeed += currentChargeAccel;
        if (chargeSpeed > currentMaxChargeSpeed) chargeSpeed = currentMaxChargeSpeed;

        vx = std::cos(chargeAngle) * chargeSpeed;
        vy = std::sin(chargeAngle) * chargeSpeed;

        if (allFrames.size() >= 8)
            setFrame(allFrames[7], vx < 0);

        if (this->x() < 0 || this->x() > sceneW - frameSize) {
            vx *= -1; chargeAngle = std::atan2(vy, vx);
        }
        if (this->y() < 0 || this->y() > sceneH - frameSize) {
            vy *= -1; chargeAngle = std::atan2(vy, vx);
        }

        chargeTimer++;
        if (chargeTimer >= chargeDuration) {
            chargeTimer = 0;
            state = COOLDOWN;
            cooldownTimer = 0;
            vx *= 0.3; vy *= 0.3;
            damage = 0;
        }
        break;
    }

    // ========== 冷却 ==========
    case COOLDOWN: {
        damage = 0;

        vx *= 0.9;
        vy *= 0.9;
        if (std::abs(vx) < 0.3) vx = 0;
        if (std::abs(vy) < 0.3) vy = 0;

        animTimer++;
        if (animTimer >= 6) {
            animTimer = 0;
            currentFrame++;
            if (currentFrame >= 7) currentFrame = 0;
            if (currentFrame < allFrames.size())
                setFrame(allFrames[currentFrame], vx < 0);
        }

        cooldownTimer++;
        if (cooldownTimer >= cooldownDuration) {
            cooldownTimer = 0;

            if (isPhase3 && chargeCount < maxCharges) {
                // 三连冲：不休息，直接进入下次前摇
                state = PREPARING;
                prepareTimer = 0;
                vx = 0; vy = 0;

                double cdx = player->x() - this->x();
                double cdy = player->y() - this->y();
                chargeAngle = std::atan2(cdy, cdx);
                if (allFrames.size() >= 8)
                    setFrame(allFrames[7], std::cos(chargeAngle) < 0);
            } else {
                // 正常回到飞行
                chargeCount = 0;
                state = FLYING;
                flyTimer = 0;
                vx = 0; vy = 0;
            }
        }
        break;
    }

    default:
        break;
    }
}
