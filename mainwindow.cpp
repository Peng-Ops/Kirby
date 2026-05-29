#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "basicenemy.h"
#include <QPainter>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->resize(1000, 600);

    // 1. 场景
    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 5000, 1000); // 地图长度设为 5000

    view = new QGraphicsView(scene, this);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 固定高度通常不需要垂直滚动条
    setCentralWidget(view);

    qreal sceneH = scene->sceneRect().height();

    // 2. 四层天空背景（修改部分：记录到 backgroundLayers）
    QStringList bgPaths = {
        ":/tu/Clouds 1/1.png",
        ":/tu/Clouds 1/2.png",
        ":/tu/Clouds 1/3.png",
        ":/tu/Clouds 1/4.png"
    };

    for(int i = 0; i < bgPaths.size(); ++i) {
        QPixmap pix(bgPaths[i]);
        if(!pix.isNull()) {
            // 将背景缩放到场景高度，保持比例
            QPixmap scaledPix = pix.scaledToHeight(sceneH, Qt::SmoothTransformation);

            // 创建一个矩形，宽度使用缩放后的图片宽度
            QGraphicsRectItem *bg = new QGraphicsRectItem(0, 0, scaledPix.width(), sceneH);
            bg->setBrush(QBrush(scaledPix));
            bg->setPen(Qt::NoPen);
            bg->setZValue(-100 + i);
            scene->addItem(bg);

            backgroundLayers.append(bg); // 记录图层
        }
    }

    // 3. 切割方块素材 (保持不变)
    QPixmap tileSheet(":/tu/fangkuai.png");
    int originalSize = 24;
    QPixmap grass        = tileSheet.copy(0, 0, originalSize, originalSize);
    QPixmap dirt         = tileSheet.copy(originalSize * 1, 0, originalSize, originalSize);
    QPixmap waterSurface = tileSheet.copy(originalSize * 2, 0, originalSize, originalSize);
    QPixmap waterBody    = tileSheet.copy(originalSize * 3, 0, originalSize, originalSize);

    // 存入类成员中，留作备用
    waterBodyPix         = tileSheet.copy(originalSize * 3, 0, originalSize, originalSize);

    // 加载冰块和碎石的素材
    QPixmap iceBlockPix(":/tu/ice.png");
    QPixmap rubblePix(":/tu/suishi.png");
    // 加载五种刺的素材
    QPixmap ciPix(":/tu/ci.png");
    QPixmap ci2Pix(":/tu/ci2.png");
    QPixmap ci3Pix(":/tu/ci3.png");
    QPixmap qiangciPix(":/tu/qiangci.png");
    QPixmap daociPix(":/tu/daoci.png");
    // ====== 新增：切割尾气素材（假设为横向两帧） ======
    QPixmap weiqiSheet(":/tu/weiqi.png");
    if (!weiqiSheet.isNull()) {
        int count = 2;
        int fw = weiqiSheet.width() / count;
        int fh = weiqiSheet.height();
        for (int i = 0; i < count; i++) {
            weiqiFrames.push_back(weiqiSheet.copy(i * fw, 0, fw, fh));
        }
    }
    // 4. 关卡矩阵 (保持不变，确保地图长度足够)
    QStringList levelData = {
        "000000000000000000000000000000000000000000000",
        "000000000000000000000000000000000000000000000",
        "000000000000000000000000000000000000000000000",
        "000000000000000000000000000000000000000000000",
        "000000000000000000000000000000000000000000000",
        "000000000000000000000000000000000000000000000",
        "000000000000000000000000000000000000000000000",
        "000000000000000000000000000000000000000000000",
        "000000000000000000000000000000000000000000000",
        "000000000000000000000000000000000000000000000",
        "000000000000000000000000000000000000000000000",
        "000000000000000000055000000006600000000000000",
        "000000000000000000055000000006600000000000000",
        "11111111111111111111111111111111111111111111111111111111111111111111111",
    };

    int renderSize = originalSize * 2;
    int totalMapHeight = levelData.size() * renderSize;
    int bottomOffset = sceneH - totalMapHeight;

    for (int r = 0; r < levelData.size(); r++) {
        for (int c = 0; c < levelData[r].length(); c++) {
            Tile *tile = nullptr;
            char type = levelData[r][c].toLatin1();
            if (type == '1')       tile = new Tile(Tile::Grass, grass);
            else if (type == '2')  tile = new Tile(Tile::Dirt, dirt);
            else if (type == '3')  tile = new Tile(Tile::WaterSurface, waterSurface);
            else if (type == '4')  tile = new Tile(Tile::WaterBody, waterBody);
            else if (type == '5')  tile = new Tile(Tile::IceBlock, iceBlockPix);
            else if (type == '6')  tile = new Tile(Tile::RubbleBlock, rubblePix);
            else if (type == '7')  tile = new Tile(Tile::Spike, ciPix);
            else if (type == '8')  tile = new Tile(Tile::Spike, ci2Pix);
            else if (type == '9')  tile = new Tile(Tile::Spike, ci3Pix);
            else if (type == 'A')  tile = new Tile(Tile::Spike, qiangciPix);
            else if (type == 'B')  tile = new Tile(Tile::Spike, daociPix);
            if (tile) {
                tile->setPos(c * renderSize, r * renderSize + bottomOffset);
                tile->setScale(2.0);
                scene->addItem(tile);
                if (type == '1' || type == '2' || type == '5' || type == '6') floors.append(tile);
                if (type == '7' || type == '8' || type == '9' || type == 'A' || type == 'B') {
                    floors.append(tile);  // 刺也能站上去（物理碰撞）
                    spikes.append(tile);  // 但对玩家造成伤害
                }
            }
        }
    }

    // 5. 卡比
    player = new Player();
    player->setPos(800, 904);
    scene->addItem(player);
    //生成敌人！
    MinionEnemy* fireEnemy = new MinionEnemy(":/tu/fire_enemy.png", 5, 1.5, Enemy::FIRE);
    fireEnemy->setScale(2);
    fireEnemy->setPos(1200, 500);
    fireEnemy->setVisible(false);
    scene->addItem(fireEnemy);
    enemies.append(fireEnemy);

    MinionEnemy* iceEnemy = new MinionEnemy(":/tu/Ice_Dude.png", 6, 1.2, Enemy::ICE);
    iceEnemy->setScale(0.6);
    iceEnemy->setPos(1600, 500);
    iceEnemy->setVisible(false);
    scene->addItem(iceEnemy);
    enemies.append(iceEnemy);

    MinionEnemy* leafEnemy = new MinionEnemy(":/tu/Leaf_Dude.png", 8, 1.0, Enemy::LEAF);
    leafEnemy->setScale(0.6);
    leafEnemy->setPos(2000, 500);
    leafEnemy->setVisible(false);
    scene->addItem(leafEnemy);
    enemies.append(leafEnemy);

    MinionEnemy* lightningEnemy = new MinionEnemy(":/tu/Lightning_Dude.png", 6, 1.8, Enemy::SPARK);
    lightningEnemy->setScale(0.6);
    lightningEnemy->setPos(2400, 500);
    lightningEnemy->setVisible(false);
    scene->addItem(lightningEnemy);
    enemies.append(lightningEnemy);

    // 猪鲨Boss
    dukeFishron = new DukeFishron(player);
    dukeFishron->setPos(3000, 400);
    dukeFishron->setVisible(false);
    scene->addItem(dukeFishron);
    enemies.append(dukeFishron);

    // 克苏鲁之脑Boss
    brainOfCthulhu = new BrainOfCthulhu(player);
    brainOfCthulhu->setPos(4200, 400);
    brainOfCthulhu->setVisible(false);
    scene->addItem(brainOfCthulhu);
    enemies.append(brainOfCthulhu);

    // 冰雪之神Boss
    iceGod = new IceGod(player);
    iceGod->setPos(3600, 400);
    iceGod->setVisible(false);
    scene->addItem(iceGod);
    enemies.append(iceGod);
    // 初始召唤4个雪花
    iceGod->summonXuehuas(4);
    for (Xuehua* x : iceGod->pendingXuehuas) {
        scene->addItem(x);
        enemies.append(x);
    }
    iceGod->pendingXuehuas.clear();

    // Boss血条
    bossHpBarBg = new QGraphicsRectItem(0, 0, 100, 8);
    bossHpBarBg->setBrush(QBrush(QColor(60, 60, 60)));
    bossHpBarBg->setPen(Qt::NoPen);
    bossHpBarBg->setZValue(2000);
    bossHpBarBg->setVisible(false);
    scene->addItem(bossHpBarBg);

    bossHpBarFg = new QGraphicsRectItem(0, 0, 100, 8);
    bossHpBarFg->setBrush(QBrush(QColor(220, 30, 30)));
    bossHpBarFg->setPen(Qt::NoPen);
    bossHpBarFg->setZValue(2001);
    bossHpBarFg->setVisible(false);
    scene->addItem(bossHpBarFg);
    // 6. 游戏循环
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::gameUpdate);

    //初始化生命值 HUD 图标
    QPixmap lifePix(":/tu/life.png");
    for (int i = 0; i < 3; ++i) {
        QGraphicsPixmapItem* icon = new QGraphicsPixmapItem(lifePix);
        icon->setZValue(1000); // 确保图层在最上方，不被背景或地图遮挡
        icon->setScale(2.0);   // 如果图片太小，可以像这样放大2倍显示
        icon->setVisible(false);
        scene->addItem(icon);
        lifeIcons.append(icon);
    }
    // ====== 技能冷却HUD (右下角) ======
    cooldownText = new QGraphicsTextItem();
    cooldownText->setFont(QFont("SimHei", 16, QFont::Bold));
    cooldownText->setDefaultTextColor(QColor(255, 255, 200)); // 淡黄色
    cooldownText->setZValue(2000);
    cooldownText->setVisible(false);
    scene->addItem(cooldownText);

    // ====== 新增：在场景中生成一个测试蛋糕 ======
    Cake* testCake = new Cake();
    testCake->setPos(250, 600);
    testCake->setVisible(false);
    scene->addItem(testCake);
    cakes.append(testCake);
    // ====== 放置检查点 ======
    Checkpoint* cp1 = new Checkpoint();
    cp1->setPos(600, 800);
    cp1->setVisible(false);
    cp1->setScale(2.0);
    scene->addItem(cp1);
    checkpoints.append(cp1);

    Checkpoint* cp2 = new Checkpoint();
    cp2->setPos(1800, 800);
    cp2->setVisible(false);
    cp2->setScale(2.0);
    scene->addItem(cp2);
    checkpoints.append(cp2);

    Checkpoint* cp3 = new Checkpoint();
    cp3->setPos(3200, 800);
    cp3->setVisible(false);
    cp3->setScale(2.0);
    scene->addItem(cp3);
    checkpoints.append(cp3);

    // 初始复活位置 = 卡比出生点
    lastCheckpointPos = QPointF(800, 904);
    titleText = new QGraphicsTextItem("星之卡比");
    titleText->setFont(QFont("SimHei", 48, QFont::Bold));
    titleText->setDefaultTextColor(Qt::white);
    titleText->setZValue(2000);
    titleText->setPos(380, 650); // 回到屏幕中央上方
    scene->addItem(titleText);

    hintText = new QGraphicsTextItem("- 按ENTER开始游戏 -");
    hintText->setFont(QFont("SimHei", 20, QFont::Bold));
    hintText->setDefaultTextColor(Qt::yellow);
    hintText->setZValue(2000);
    hintText->setPos(380, 780);  // 回到屏幕中央下方
    scene->addItem(hintText);

    // 音乐
    bgmPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    bgmPlayer->setAudioOutput(audioOutput);
    bgmPlayer->setSource(QUrl("qrc:///tu/kerby theme music.mp3"));
    audioOutput->setVolume(0.5); // 音量范围 0.0 到 1.0
    bgmPlayer->setLoops(QMediaPlayer::Infinite); // 无限循环
    bgmPlayer->play();

    // 6. 游戏循环
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::gameUpdate);
    timer->start(16);
}
MainWindow::~MainWindow() { delete ui; }

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (currentState == START_SCREEN) {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            enterPressed = true;
            /*currentState = INTRO_PAN;
            if (titleText) { scene->removeItem(titleText); delete titleText; titleText = nullptr; }
            if (hintText) { scene->removeItem(hintText); delete hintText; hintText = nullptr; }*/
        }
        return;
    }

    if (currentState == INTRO_PAN) return; // 动画期间拦截所有按键

    if (event->isAutoRepeat()) return;
    if (player->isDigesting) {
        return;
    }
    // ====== 核心修复点：处于 Fatty 状态时，移除对 L 键的拦截 ======
    if (player->isFatty || player->isSpitting) {
        // 这里去掉了原本的 || event->key() == Qt::Key_L
        // 这样变胖时，按下 L 键才不会被中途拦截，能顺利传到底部触发变身
        if (event->key() == Qt::Key_W || event->key() == Qt::Key_Up ||
            event->key() == Qt::Key_J || event->key() == Qt::Key_K) {
            return;
        }
    }

    if (player->isSwallowing) {
        if (event->key() != Qt::Key_L) return;
    }

    keys.insert(event->key());

    if (event->key() == Qt::Key_A || event->key() == Qt::Key_Left)
        lastHorizontalKey = event->key();
    else if (event->key() == Qt::Key_D || event->key() == Qt::Key_Right)
        lastHorizontalKey = event->key();
    else if (event->key() == Qt::Key_W || event->key() == Qt::Key_Up){
        if (player->currentForm != Enemy::SPARK) {
            jumpBuffer = 6;
        }
    }
    else if (event->key() == Qt::Key_K)
        player->startRoll();
    // ====== J键攻击——四形态天生攻击，普通形态需蛋糕 ======
    else if (event->key() == Qt::Key_J) {
        // 增加对疾跑和爆炸状态的拦截
        bool canAttack = !player->isAttacking && !player->isLeafSkill && !player->isRolling &&
                         !player->isSwallowing && !player->isSpitting &&
                         !player->isFireSprinting && !player->isExploding;
        if (!canAttack) return;

        Enemy::CopyAbility form = player->currentForm;

        // 核心修改：只有普通形态才能发动原本的光球攻击
        if (form == Enemy::NONE) {
            if (player->hasAttackPower()) {
                player->startAttack();
                Projectile* proj = new Projectile(player->facingRight);
                double startX = player->facingRight ? player->x() + 48 : player->x() - 24;
                double startY = player->y() + 12;
                proj->setPos(startX, startY);
                scene->addItem(proj);
                projectiles.append(proj);
            }
        }
        // 核心修改：火形态按下 J 触发专属疾跑
        else if (form == Enemy::FIRE) {
            if (player->fireSkillCooldownTimer <= 0) {
                player->startFireSprint();
            }
        }
        // 3. 叶形态：发射羽毛技能
        else if (form == Enemy::LEAF) {
            if (player->leafSkillCooldownTimer <= 0) {
                player->startLeafSkill();
                player->leafSkillCooldownTimer = 180; // 3秒冷却

                Projectile* proj = new Projectile(player->facingRight);
                proj->damage = 1;

                QPixmap featherPix(":/tu/feather.png");

                // 【核心修复】：强制缩小羽毛高度为 20 像素，去除多余的透明边缘碰撞
                featherPix = featherPix.scaledToHeight(20, Qt::SmoothTransformation);

                if (!player->facingRight) {
                    featherPix = featherPix.transformed(QTransform().scale(-1, 1));
                }
                proj->setPixmap(featherPix);

                // 【核心修复】：把 Y 坐标调到卡比正中央 (卡比高48，中间就是24)
                double startX = player->facingRight ? player->x() + 48 : player->x() - featherPix.width();
                double startY = player->y() + 24 - (featherPix.height() / 2.0);
                proj->setPos(startX, startY);

                scene->addItem(proj);
                projectiles.append(proj);
            }
        }
        // 4. ====== 新增：电形态切换飞行模式 ======
        else if (form == Enemy::SPARK) {
            player->isLightningFlying = !player->isLightningFlying;
            if (player->isLightningFlying) {
                jumpBuffer = 0;
                player->vy = 0; // 开启时瞬间悬停，抵消掉落惯性
            } else {
                player->setState(Player::JUMPING); // 关掉时恢复自然下落动作
            }
        }
    }
    else if (event->key() == Qt::Key_T) {
        if (player->isFatty && !player->isSpitting) {
            player->startSpit();
        }
        // 2. ====== 新增：如果当前拥有变身形态，按 T 直接取消变身 ======
        else if (!player->isFatty && player->currentForm != Enemy::NONE) {
            player->currentForm = Enemy::NONE; // 瞬间恢复为普通形态

            // 强制中断所有形态专属的技能状态，防止卡死在半空或保持异常移速
            player->isAttacking = false;
            player->isRolling = false;
            player->isFireSprinting = false;
            player->isExploding = false;

            // 重置为常规的待机或跳跃状态
            player->setState(player->isOnGround ? Player::IDLE : Player::JUMPING);
        }
    }
    // ====== L键多功能逻辑 ======
    else if (event->key() == Qt::Key_L) {
        // 1. 如果是胖子状态，按 L 消化变身
        if (player->isFatty && !player->isSpitting && !player->isSwallowing) {
            player->startDigest();
        }
        // 2. 如果已拥有形态能力，启动长按计时器（需按住1秒才取消，防误触）
        else if (!player->isFatty && player->currentForm != Enemy::NONE && player->formCancelTimer == 0) {
            player->formCancelTimer = 1;
        }
    }
    // ====== J键长按：冰形态专属防御 ======
    if (keys.contains(Qt::Key_J) && player->currentForm == Enemy::ICE && player->iceDefendCooldownTimer <= 0) {
        if (!player->isRolling && !player->isSwallowing && !player->isDigesting && !player->isSpitting && !player->isIceDefending) {
            player->startIceDefend();
        }
    } else if (!keys.contains(Qt::Key_J) && player->isIceDefending) {
        // 松开 J 键时，立刻解除防御状态并开始 10 秒冷却
        player->endIceDefend();
    }
}
void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    keys.remove(event->key());
    if (event->key() == lastHorizontalKey)
        lastHorizontalKey = 0;
}

void MainWindow::gameUpdate() {
    // 阶段一：静止的开始界面
    if (currentState == START_SCREEN) {
        if (enterPressed) {
            player->vx = 0; // 左右停住，不让它乱跑

            // 物理系统会自动让它下落，一旦碰撞检测判定它踩地了
            if (player->isOnGround) {
                enterPressed = false;      // 重置标记
                if (titleText) { scene->removeItem(titleText); delete titleText; titleText = nullptr; }
                if (hintText) { scene->removeItem(hintText); delete hintText; hintText = nullptr; }
                currentState = INTRO_PAN;     // 【关键】此时才真正切换到屏幕滑动状态
            }
        }
        // 情况 B：正常挂机状态，卡比随节拍随机乱动
        else {
            aiTimer++;

            // 【节奏感核心】：假设游戏 60帧/秒，每 45 帧（约0.75秒，类似一个节拍）改变一次动作
            if (aiTimer % 26 == 0) {
                player->isRolling = false;
                int randNum = rand() % 100;

                if (randNum < 20) {
                    // 20% 概率向左走
                    player->vx = -3;
                    player->facingRight = false;
                }
                else if (randNum < 40) {
                    // 20% 概率向右走
                    player->vx = 3;
                    player->facingRight = true;
                }
                else if (randNum < 70) {
                    // 30% 概率跳跃
                    if (player->isOnGround) {
                        player->vy = -16;
                        player->isOnGround = false;
                    }
                }
                else if (randNum < 85) {
                    // 15% 概率滚动
                    if (player->isOnGround) {
                        player->isRolling = true; // 开启滚动状态（触发滚动动画）
                        player->vx = player->facingRight ? 8 : -8;
                        player->resetRollAnim();
                    } else {
                        player->vx = 0; // 如果在空中就不滚了，原地发呆
                    }
                }
                else {
                    // 15% 概率呆着不动
                    player->vx = 0;
                }
            }

            // 【范围限制】：别让卡比走出初始屏幕
            if (player->x() < 200) {
                player->vx = 3;
                player->facingRight = true;
            }
            if (player->x() > 800) {
                player->vx = -3;
                player->facingRight = false;
            }
        }

        if (!player->isOnGround) {
            player->vy += 0.8;          // 正常的重力加速度
            if (player->vy > 15) player->vy = 15; // 限制最大下落速度
        }

        player->setPos(player->x() + player->vx, player->y() + player->vy);

        if (player->y() >= 904) {
            player->setPos(player->x(), 904);
            player->vy = 0;
            player->isOnGround = true;
        }

        player->updateLogic();
        // 锁定镜头
        view->centerOn(500, 850);
        for (QGraphicsRectItem* bg : backgroundLayers) {
            bg->setPos(500 - bg->rect().width() / 2.0, 0);
        }
        return;
    }
    // 阶段二：按下回车，卡比和地面一起左移
    if (currentState == INTRO_PAN) {
        player->isOnGround = true;
        qreal moveSpeed = 8.0;
        player->vx = 0; player->vy = 0;
        player->setPos(player->x() - moveSpeed, 904); // 贴地平移

        // 👇 核心修改 1：地板、怪物和道具一起跟着向左平移，保持相对地图的绝对位置
        for (Tile* tile : floors) {
            tile->setPos(tile->x() - moveSpeed, tile->y());
        }
        for (Enemy* e : enemies) {
            e->setPos(e->x() - moveSpeed, e->y());
        }
        for (Cake* c : cakes) {
            c->setPos(c->x() - moveSpeed, c->y());
        }

        // 到达原定位置
        if (player->x() <= 100) {
            qreal errorX = 100 - player->x();
            // 👇 核心修改 2：最后一帧对齐误差校准时，怪物和道具也要同步校准
            for (Tile* tile : floors) {
                tile->setPos(tile->x() + errorX, tile->y());
            }
            for (Enemy* e : enemies) {
                e->setPos(e->x() + errorX, e->y());
            }
            for (Cake* c : cakes) {
                c->setPos(c->x() + errorX, c->y());
            }
            player->setPos(100, 904); // 完美归位

            // 切换状态，唤醒所有元素
            currentState = PLAYING;
            for (Enemy* e : enemies) e->setVisible(true);
            for (Cake* c : cakes) c->setVisible(true);
            for (Checkpoint* cp : checkpoints) cp->setVisible(true);
        }

        player->updateLogic();
        view->centerOn(500, 850);
        for (QGraphicsRectItem* bg : backgroundLayers) {
            bg->setPos(500 - bg->rect().width() / 2.0, 0);
        }
        return;
    }

    // ====== L键长按：吞噬 vs 取消形态 ======
    if (keys.contains(Qt::Key_L) && player->currentForm == Enemy::NONE && player->isOnGround && !player->isFatty && !player->isRolling && !player->isAttacking && !player->isDigesting && !player->isSpitting) {
        player->startSwallow();
    } else if ((!keys.contains(Qt::Key_L) || !player->isOnGround) && player->isSwallowing) {
        player->endSwallow();
    }

    // 长按L取消形态（需持续按住60帧=1秒，松手即重置）
    if (player->formCancelTimer > 0) {
        if (keys.contains(Qt::Key_L)) {
            player->formCancelTimer++;
            if (player->formCancelTimer >= 60) {
                player->currentForm = Enemy::NONE;
                player->formCancelTimer = 0;
            }
        } else {
            player->formCancelTimer = 0; // 松手就重置，防误触
        }
    }

    // ====== 运动状态速度驱动 ======
    if (player->isRolling) {
        player->vx = player->facingRight ? Player::rollSpeed : -Player::rollSpeed;

        if (player->rollTimer >= 0) {
            if (jumpBuffer > 0) {
                if (player->isOnGround) {
                    player->vy = -16; coyoteTime = 0; jumpBuffer = 0; player->endRoll();
                } else if (player->canDoubleJump) {
                    player->vy = -15; player->canDoubleJump = false; jumpBuffer = 0; player->endRoll();
                }
            }
        } else {
            jumpBuffer = 0; coyoteTime = 0; player->isHovering = false;
        }
    }
    else if (player->isSwallowing) {
        player->vx = 0;
        jumpBuffer = 0; // 核心：吞噬时清除跳跃缓冲，防止吞噬结束后突然起跳
        coyoteTime = 0; // 核心：清除土狼时间
    }
    // ====== 新增：吐出动画播放期间，水平移动物理速度归零 ======
    else if (player->isSpitting) {
        player->vx = 0;
        jumpBuffer = 0;
        coyoteTime = 0;
    }
    // ====== 新增：在播放变身动画期间，物理移动速度、跳跃缓冲全部归零 ======
    else if (player->isDigesting) {
        player->vx = 0;
        jumpBuffer = 0;
        coyoteTime = 0;
    }
    // ====== 修改这部分：锁定爆炸期间的移动 ======
    else if (player->isExploding || player->isAttacking || player->isLeafSkill || player->isLightningFlying|| player->isIceDefending) {
        jumpBuffer = 0; coyoteTime = 0;
        if (player->isExploding) {
            player->vx = 0; player->vy = 0;
        }
        else if (player->isAttacking || player->isLeafSkill) {
            player->vx = 0;
        }
        // ====== 闪电飞行的八向移动（统一速度） ======
        else if (player->isLightningFlying) {
            jumpBuffer = 0; coyoteTime = 0;
            int flySpeed = 6;

            // 处理水平
            if (keys.contains(Qt::Key_A) || keys.contains(Qt::Key_Left)) {
                player->vx = -flySpeed; player->facingRight = false;
            } else if (keys.contains(Qt::Key_D) || keys.contains(Qt::Key_Right)) {
                player->vx = flySpeed; player->facingRight = true;
            } else {
                player->vx = 0;
            }

            // 处理垂直
            if (keys.contains(Qt::Key_W) || keys.contains(Qt::Key_Up)) {
                player->vy = -flySpeed;
            } else if (keys.contains(Qt::Key_S) || keys.contains(Qt::Key_Down)) {
                player->vy = flySpeed;
            } else {
                player->vy = 0;
            }
        }
    }
    else {
        // 水平输入（Fatty 状态下正常允许走动）
        int activeKey = 0;
        if (keys.contains(lastHorizontalKey)) activeKey = lastHorizontalKey;
        else {
            if (keys.contains(Qt::Key_A) || keys.contains(Qt::Key_Left)) activeKey = Qt::Key_A;
            else if (keys.contains(Qt::Key_D) || keys.contains(Qt::Key_Right)) activeKey = Qt::Key_D;
        }

        if (activeKey == Qt::Key_A || activeKey == Qt::Key_Left) {
            player->vx = -5; player->facingRight = false;
        } else if (activeKey == Qt::Key_D || activeKey == Qt::Key_Right) {
            player->vx = 5; player->facingRight = true;
        } else {
            player->vx = 0;
        }
    }
    // ====== 翻滚状态特殊处理 ======
    if (player->isRolling) {
        player->vx = player->facingRight ? Player::rollSpeed : -Player::rollSpeed;

        if (player->rollTimer>=0) {
            // 允许跳跃打断
            if (jumpBuffer > 0) {
                if (player->isOnGround) {
                    player->vy = -16;
                    coyoteTime = 0;
                    jumpBuffer = 0;
                    player->endRoll();
                } else if (player->canDoubleJump) {
                    player->vy = -15;
                    player->canDoubleJump = false;
                    jumpBuffer = 0;
                    player->endRoll();
                }
            }
        } else {
            jumpBuffer = 0;
            coyoteTime = 0;
            player->isHovering = false;
        }
    } else if (!player->isSwallowing && !player->isIceDefending && !player->isSpitting && !player->isDigesting && !player->isFireSprinting) {
        // 1. 水平输入（原有逻辑）—— 吞噬/吐出/变身时跳过，防止覆盖第一段运动逻辑的vx=0
        int activeKey = 0;
        if (keys.contains(lastHorizontalKey)) activeKey = lastHorizontalKey;
        else {
            if (keys.contains(Qt::Key_A) || keys.contains(Qt::Key_Left)) activeKey = Qt::Key_A;
            else if (keys.contains(Qt::Key_D) || keys.contains(Qt::Key_Right)) activeKey = Qt::Key_D;
        }

        if (activeKey == Qt::Key_A || activeKey == Qt::Key_Left) {
            player->vx = -5; player->facingRight = false;
        } else if (activeKey == Qt::Key_D || activeKey == Qt::Key_Right) {
            player->vx = 5; player->facingRight = true;
        } else {
            player->vx = 0;
        }
    }

    // 2. 重力机制拦截
    if (player->isFatty) {
        player->isHovering = false; // Fatty 绝对禁止悬浮
    } else {
        player->isHovering = (keys.contains(Qt::Key_W) || keys.contains(Qt::Key_Up))
        && !player->isRolling && !player->isSwallowing;
    }

    if (!player->isOnGround&& !player->isLightningFlying) {
        if (player->isHovering && player->vy > 0) {
            player->vy += 0.1;
            if (player->vy > 2.5) player->vy = 2.5;
        } else {
            player->vy += 0.8;
            if (player->vy > 15) player->vy = 15;
        }
    }

    // 3. 土狼时间与跳跃缓冲
    if (player->isFatty) {
        // 彻底清洗所有与离地跳跃相关的变量
        jumpBuffer = 0;
        coyoteTime = 0;
        player->canDoubleJump = false;
    } else if (!player->isRolling && !player->isSwallowing) {
        if (player->isOnGround) {
            coyoteTime = 6;
            player->canDoubleJump = true;
        } else {
            if (coyoteTime > 0) coyoteTime--;
        }

        if (jumpBuffer > 0) {
            if (coyoteTime > 0) {
                player->vy = -16; coyoteTime = 0; jumpBuffer = 0;
            } else if (player->canDoubleJump) {
                player->vy = -15; player->canDoubleJump = false; jumpBuffer = 0;
                player->currentState = Player::IDLE;
            }
            if (jumpBuffer > 0) jumpBuffer--;
        }
    }
    // 4. 水平移动 + 碰撞
    player->setPos(player->x() + player->vx, player->y());

    // 替换原有的 constFloors 遍历为倒序遍历
    for (int i = floors.size() - 1; i >= 0; i--) {
        Tile *tile = floors[i];
        if (player->collidesWithItem(tile)) {

            // ====== 技能互动：火形态疾跑撞击特殊方块 ======
            if (player->isFireSprinting) {
                if (tile->tileType() == Tile::IceBlock) {
                    // 火融冰：改变地形贴图，由于是水了，不产生物理阻挡，直接 continue
                    tile->changeType(Tile::WaterBody, waterBodyPix);
                    floors.removeAt(i);
                    continue;
                }
                else if (tile->tileType() == Tile::RubbleBlock) {
                    // 撞碎石：自己爆炸，销毁石头，不产生物理阻挡
                    player->startExplosion();
                    scene->removeItem(tile);
                    floors.removeAt(i);
                    delete tile;
                    continue;
                }
            }

            // 正常的物理阻挡逻辑
            QRectF tRect = tile->sceneBoundingRect();
            if (player->vx > 0) {
                player->setPos(tRect.left() - 48, player->y());
                if (player->isFireSprinting) player->endFireSprint(); // 撞到普通硬墙，被迫停下
            } else if (player->vx < 0) {
                player->setPos(tRect.right(), player->y());
                if (player->isFireSprinting) player->endFireSprint();
            }
        }
    }

    // 5. 垂直检测
    bool onGround = false;

    // A. 预判地面 (向下探测1像素)
    player->setPos(player->x(), player->y() + 1);
    for (int i = floors.size() - 1; i >= 0; i--) {
        Tile *tile = floors[i];
        if (player->collidesWithItem(tile)) {

            // ====== 技能互动：火形态疾跑上下方向融冰与炸石 ======
            if (player->isFireSprinting) {
                if (tile->tileType() == Tile::IceBlock) {
                    // 火融冰：改变地形贴图并移除物理体积
                    tile->changeType(Tile::WaterBody, waterBodyPix);
                    floors.removeAt(i);
                    continue;
                } else if (tile->tileType() == Tile::RubbleBlock) {
                    // 踩/顶到碎石：自己爆炸，销毁石头
                    player->startExplosion();
                    scene->removeItem(tile);
                    floors.removeAt(i);
                    delete tile;
                    continue;
                }
            }

            // 原本的正常阻挡逻辑
            if (player->vy >= 0) {
                QRectF tRect = tile->sceneBoundingRect();
                player->setPos(player->x(), tRect.top() - 48); // 将 pRect.height() 替换为固定的 48
                player->vy = 0;
                onGround = true;
            }
            break; // 踩到一块实心方块就跳出检测
        }
    }

    // B. 正式垂直移动与碰撞
    if (!onGround) {
        player->setPos(player->x(), player->y() - 1 + player->vy);
        for (int i = floors.size() - 1; i >= 0; i--) {
            Tile *tile = floors[i];
            if (player->collidesWithItem(tile)) {

                // ====== 技能互动：火形态疾跑上下方向融冰与炸石 ======
                if (player->isFireSprinting) {
                    if (tile->tileType() == Tile::IceBlock) {
                        tile->changeType(Tile::WaterBody, waterBodyPix);
                        floors.removeAt(i);
                        continue;
                    } else if (tile->tileType() == Tile::RubbleBlock) {
                        player->startExplosion();
                        scene->removeItem(tile);
                        floors.removeAt(i);
                        delete tile;
                        continue;
                    }
                }

                // 原本的正常阻挡逻辑
                QRectF tRect = tile->sceneBoundingRect();
                if (player->vy >= 0) {
                    player->setPos(player->x(), tRect.top() - 48);
                    player->vy = 0;
                    onGround = true;
                } else {
                    player->setPos(player->x(), tRect.bottom()); // 撞头
                    player->vy = 0;
                }
            }
        }
    }
    player->isOnGround = onGround;
    // 6. 翻滚结束判定
    if (player->isRolling) {
        player->rollTimer--;
        if (player->rollTimer <= 0) {
            player->endRoll();
        }
    }

    // 7. 状态更新与视角
    player->isHovering = (keys.contains(Qt::Key_W) || keys.contains(Qt::Key_Up))
                         && !player->isRolling;  // 翻滚时禁止漂浮
    player->updateLogic();
    view->centerOn(player);


    // ====== 运动方块更新（会动的刺等） ======
    for (Tile* tile : floors) {
        tile->updateLogic();
    }

    // ====== 敌人物理与逻辑计算 ======
    for (Enemy* enemy : enemies) {
        if (enemy->isDead) continue;

        // 1. 更新敌人自身逻辑 (决定 vx 和动画)
        enemy->updateLogic();

        // 2. 水平移动与墙壁碰撞（Boss无视地形）
        enemy->setPos(enemy->x() + enemy->vx, enemy->y());
        if (!enemy->ignoresTiles) {
            QRectF eRect = enemy->sceneBoundingRect();
            for (Tile *tile : floors) {
                if (enemy->collidesWithItem(tile)) {
                    QRectF tRect = tile->sceneBoundingRect();
                    // 物理阻挡
                    if (enemy->vx > 0) {
                        enemy->setPos(tRect.left() - eRect.width(), enemy->y());
                    } else if (enemy->vx < 0) {
                        enemy->setPos(tRect.right(), enemy->y());
                    }
                    // 撞墙后掉头
                    enemy->reverseDirection();
                    break;
                }
            }

            // 3. 应用重力 (自由落体算法)
            enemy->vy += 0.8;
            if (enemy->vy > 15) enemy->vy = 15; // 终端速度限制

            // 4. 垂直移动与地面碰撞
            enemy->setPos(enemy->x(), enemy->y() + enemy->vy);
            eRect = enemy->sceneBoundingRect();
            for (Tile *tile : floors) {
                if (enemy->collidesWithItem(tile)) {
                    QRectF tRect = tile->sceneBoundingRect();
                    if (enemy->vy > 0) { // 往下掉时踩到地板
                        enemy->setPos(enemy->x(), tRect.top() - eRect.height());
                        enemy->vy = 0; // 落地速度清零
                    }
                    break;
                }
            }
        } else {
            // Boss无视地形，直接应用vy（飞行类Boss自己控制vy）
            enemy->setPos(enemy->x(), enemy->y() + enemy->vy);
        }
    }
    // ====== 相机边界限位与背景固定逻辑 ======

    // 1. 获取基础参数
    qreal cameraX = player->x();
    qreal cameraY = 850;
    qreal sceneW = scene->sceneRect().width();
    qreal halfViewW = view->viewport()->width() / 2.0;

    // 2. 边界检查
    if (cameraX < halfViewW) {
        cameraX = halfViewW;
    } else if (cameraX > sceneW - halfViewW) {
        cameraX = sceneW - halfViewW;
    }

    // 3. 执行视角居中并加入【震屏特效】
    qreal renderX = cameraX;
    qreal renderY = cameraY;

    if (player->isExploding) {
        // 使用随机数让相机在中心点周围 ±8 像素剧烈抖动
        renderX += (rand() % 17) - 8;
        renderY += (rand() % 17) - 8;
    }

    view->centerOn(renderX, renderY);

    // 4. 背景图层跟随“锁定后且未震动”的相机坐标
    // 这样背景不会跟着画面一起疯狂乱抖，产生极好的纵深对比感
    for (QGraphicsRectItem* bg : backgroundLayers) {
        qreal bgX = cameraX - bg->rect().width() / 2.0;
        bg->setPos(bgX, 0);
    }

    // ====== Boss血条（屏幕右上角HUD） ======
    // 找到当前存活且可见的Boss（猪鲨或克苏鲁之脑）
    BossEnemy* activeBoss = nullptr;
    int bossFullHp = 1;
    if (dukeFishron && !dukeFishron->isDead && dukeFishron->isVisible()) {
        activeBoss = dukeFishron;
        bossFullHp = dukeFishron->fullHp;
    } else if (brainOfCthulhu && !brainOfCthulhu->isDead && brainOfCthulhu->isVisible()) {
        activeBoss = brainOfCthulhu;
        bossFullHp = brainOfCthulhu->fullHp;
    } else if (iceGod && !iceGod->isDead && iceGod->isVisible()) {
        activeBoss = iceGod;
        bossFullHp = iceGod->fullHp;
    }
    if (activeBoss && activeBoss->hp > 0) {
        double barW = 120.0;
        double barH = 10.0;
        double barX = cameraX + halfViewW - barW - 20;
        double barY = cameraY - view->viewport()->height() / 1.2 + 20;
        bossHpBarBg->setVisible(true);
        bossHpBarBg->setRect(0, 0, barW, barH);
        bossHpBarBg->setPos(barX, barY);
        bossHpBarFg->setVisible(true);
        double ratio = (double)activeBoss->hp / bossFullHp;
        if (ratio < 0) ratio = 0;
        bossHpBarFg->setRect(0, 0, barW * ratio, barH);
        bossHpBarFg->setPos(barX, barY);
    } else {
        if (bossHpBarBg) bossHpBarBg->setVisible(false);
        if (bossHpBarFg) bossHpBarFg->setVisible(false);
    }

    // ====== 技能冷却HUD (右下角) ======
    int cooldown = 0;
    QString cdLabel;
    switch (player->currentForm) {
        case Enemy::FIRE: cooldown = player->fireSkillCooldownTimer; cdLabel = "火疾跑"; break;
        case Enemy::ICE:  cooldown = player->iceDefendCooldownTimer; cdLabel = "冰防御"; break;
        case Enemy::LEAF: cooldown = player->leafSkillCooldownTimer; cdLabel = "叶羽毛"; break;
        default: break;
    }
    if (cooldown > 0) {
        int sec = (cooldown + 59) / 60;
        cooldownText->setPlainText(QString("%1: %2s").arg(cdLabel).arg(sec));
        qreal scrRight  = cameraX + halfViewW;
        qreal scrBottom = cameraY + view->viewport()->height() / 2.0;
        qreal tw = cooldownText->boundingRect().width();
        qreal th = cooldownText->boundingRect().height();
        cooldownText->setPos(scrRight - tw - 20, scrBottom - th - 20);
        cooldownText->setVisible(true);
    } else {
        cooldownText->setVisible(false);
    }

    // ====== 消费IceGod新召唤的雪花 ======
    if (iceGod && !iceGod->isDead) {
        for (Xuehua* x : iceGod->pendingXuehuas) {
            scene->addItem(x);
            enemies.append(x);
        }
        iceGod->pendingXuehuas.clear();
    }

    // ====== 消费Boss待发射弹幕 ======
    if (brainOfCthulhu && !brainOfCthulhu->isDead) {
        for (Projectile* p : brainOfCthulhu->pendingProjectiles) {
            scene->addItem(p);
            projectiles.append(p);
        }
        brainOfCthulhu->pendingProjectiles.clear();
    }
    if (iceGod && !iceGod->isDead) {
        for (Projectile* p : iceGod->pendingProjectiles) {
            scene->addItem(p);
            projectiles.append(p);
        }
        iceGod->pendingProjectiles.clear();
    }

    // ====== 子弹逻辑与伤害判定 ======
    // 逆序遍历，方便在遍历中安全地删除元素
    for (int i = projectiles.size() - 1; i >= 0; i--) {
        Projectile* proj = projectiles[i];
        proj->updateLogic();

        bool hitEnemy = false;
        bool hitWall = false; // 新增：是否撞墙的标志位
        // 1. 检测是否打到敌人（仅限对敌人有害的弹幕，Boss弹幕不会自伤）
        for (int j = enemies.size() - 1; j >= 0; j--) {
            Enemy* enemy = enemies[j];
            if (proj->hurtsEnemies && !enemy->isDead && proj->collidesWithItem(enemy)) {
                // 扣血
                enemy->takeDamage(proj->damage);
                hitEnemy = true;

                // 敌人死亡
                if (enemy->isDead) {
                    scene->removeItem(enemy);
                    enemies.removeAt(j);
                    delete enemy;
                }
                break; // 一颗光球只能打一个敌人，打中就退出内层循环
            }
        }


        // 2. 新增：检测是否撞到实体方块
        if (!hitEnemy) { // 如果已经打中敌人了，就不需要再检测撞墙了
            for (Tile *tile : floors) {
                // 只要子弹碰到了 floors 列表里的方块（实体墙、草地、石头等）
                if (proj->collidesWithItem(tile)) {
                    hitWall = true;
                    break;
                }
            }
        }
        // 2.5 检测Boss弹幕是否打到玩家
        bool hitPlayer = false;
        if (proj->hurtsPlayer && player->invulnTimer == 0 && player->hp > 0) {
            if (proj->collidesWithItem(player)) {
                hitPlayer = true;
                if (!player->isRolling && !player->isSwallowing && !player->isIceDefending) {
                    player->hp--;
                    player->invulnTimer = 60;
                    player->vy = -4;
                }
            }
        }

        // 3. 检测光球是否寿命耗尽 或 击中了敌人/玩家/墙
        if (hitEnemy || hitPlayer || hitWall || proj->lifeTime <= 0) {
            scene->removeItem(proj);
            projectiles.removeAt(i);
            delete proj;
        }
    }
    // ====== 玩家与刺的碰撞检测 ======
    if (player->invulnTimer == 0 && player->hp > 0) {
        for (Tile* spike : spikes) {
            if (player->collidesWithItem(spike)) {
                player->hp--;
                player->invulnTimer = 60;
                player->vy = -6;
                break; // 一帧内只被刺伤一次
            }
        }
    }

    // ====== 新增：玩家与敌人碰撞检测 (受击扣血) ======
    if (player->invulnTimer == 0 && player->hp > 0) {
        // 必须用逆序遍历，因为火形态疾跑爆炸可能会直接杀死敌人并将其从内存删除
        // 但如果被刺伤到，不再检测敌人碰撞
        if (player->hp > 0) {
        for (int j = enemies.size() - 1; j >= 0; j--) {
            Enemy* enemy = enemies[j];
            // 只有活着的目标才能对玩家造成伤害
            if (!enemy->isDead && player->collidesWithItem(enemy)) {

                // 【机制平衡】如果卡比正在翻滚或吞噬，视为无敌/免伤，不扣血
                if (player->isRolling) continue;
                if (player->isSwallowing) continue;
                if (player->isIceDefending) continue;
                // ====== 技能互动：火形态疾跑或爆炸触碰敌人 ======
                if (player->isFireSprinting || player->isExploding) {
                    if (player->isFireSprinting) {
                        player->startExplosion(); // 疾跑撞到人触发爆炸
                    }

                    enemy->takeDamage(2); // 爆炸对敌人造成2点伤害
                    if (enemy->isDead) {
                        scene->removeItem(enemy);
                        enemies.removeAt(j);
                        delete enemy;
                    }
                    continue; // 爆炸期间具有绝对无敌免伤，跳过下方扣血代码
                }

                player->hp--;
                player->invulnTimer = 60;
                player->vy = -6;
                break; // 单帧内只承受一次伤害
            }
        }
        } // if (player->hp > 0) 被刺伤后跳过敌人检测
    }

    // ====== 统一死亡检查（刺或敌人都可能导致死亡）======
    if (player->hp <= 0) {
        timer->stop();

        QGraphicsTextItem* gameOverText = new QGraphicsTextItem("GAME OVER");
        gameOverText->setFont(QFont("SimHei", 48, QFont::Bold));
        gameOverText->setDefaultTextColor(Qt::red);
        gameOverText->setZValue(2000);
        qreal goX = cameraX - 180;
        qreal goY = cameraY - view->viewport()->height() / 3;
        gameOverText->setPos(goX, goY);
        scene->addItem(gameOverText);

        QTimer::singleShot(1500, this, [this, gameOverText]() {
            QMessageBox::StandardButton reply =
                QMessageBox::question(this, "Game Over", "是否要复活？");
            scene->removeItem(gameOverText);
            delete gameOverText;

            if (reply == QMessageBox::Yes) {
                player->setPos(lastCheckpointPos.x(), lastCheckpointPos.y());
                player->hp = 3;
                player->invulnTimer = 60;
                player->vy = 0;
                player->vx = 0;
                player->currentForm = Enemy::NONE;
                player->isFatty = false;
                player->attackPowerTimer = 0;
                player->isAttacking = false;
                player->isRolling = false;
                player->isSwallowing = false;
                player->isSpitting = false;
                player->isDigesting = false;
                player->isLeafSkill = false;
                player->isLightningFlying = false;
                player->isIceDefending = false;
                player->isExploding = false;
                player->isFireSprinting = false;
                player->isHovering = false;
                timer->start();
            } else {
                this->close();
            }
        });
        return;
    }

    // ====== 检查点碰撞检测 ======
    for (Checkpoint* cp : checkpoints) {
        if (!cp->isActivated && player->collidesWithItem(cp)) {
            cp->activate();
            lastCheckpointPos = cp->pos();
            hasCheckpoint = true;
        }
    }

    // ====== 新增：生命值图标动态固定在屏幕左上角 (HUD) ======
    qreal screenLeft = cameraX - halfViewW;
    qreal screenTop = cameraY - view->viewport()->height()/1.2;

    for (int i = 0; i < lifeIcons.size(); ++i) {
        if (i < player->hp) {
            lifeIcons[i]->setVisible(true);
            // 依次排列在屏幕左上角，留出 20 像素边距，每个图标间隔 40 像素
            lifeIcons[i]->setPos(screenLeft + 20 + i * 40, screenTop + 20);
        } else {
            // 命扣掉了就不显示
            lifeIcons[i]->setVisible(false);
        }
    }
    // ====== 修改后：同时支持吞噬怪物与拉扯蛋糕的吞噬逻辑 ======
    if (player->isSwallowing) {
        // 1. 获取卡比 48x48 的物理身体矩形
        QRectF playerRect(player->x(), player->y(), 48, 48);

        // 2. 建立吸力矩形：起点在卡比身上，往面朝方向延伸 60 像素
        QRectF swallowWindRect;
        if (player->facingRight) {
            swallowWindRect = QRectF(player->x(), player->y(), 48 + 60, 48);
        } else {
            swallowWindRect = QRectF(player->x() - 60, player->y(), 60 + 48, 48);
        }

        // 3. 逆序检查场景中的怪物（拉扯与进肚）
        for (int j = enemies.size() - 1; j >= 0; j--) {
            Enemy* enemy = enemies[j];
            if (!enemy->isDead && enemy->canBeSwallowed()) {
                QRectF enemyRect = enemy->sceneBoundingRect();

                // 【怪物阶段一】：触碰身体 -> 真正吸进肚子里
                if (playerRect.intersects(enemyRect)) {
                    player->swallowedAbility = enemy->ability; // 记住能力
                    enemy->isDead = true;
                    scene->removeItem(enemy);
                    enemies.removeAt(j);
                    delete enemy;

                    player->isFatty = true; // 吞到怪物，进入变胖状态
                    player->endSwallow();
                    break;
                }
                // 【怪物阶段二】：处于吸气范围内 -> 强行拉扯
                else if (swallowWindRect.intersects(enemyRect)) {
                    double pullSpeed = 5.5;
                    if (player->facingRight) {
                        enemy->setPos(enemy->x() - pullSpeed, enemy->y());
                    } else {
                        enemy->setPos(enemy->x() + pullSpeed, enemy->y());
                    }
                }
            }
        }

        // 4. 【全新添加】：逆序检查场景中的蛋糕（拉扯与进肚）
        for (int j = cakes.size() - 1; j >= 0; j--) {
            Cake* cake = cakes[j];
            QRectF cakeRect = cake->sceneBoundingRect();

            // 【蛋糕阶段一】：蛋糕触碰到了卡比的物理身体 -> 真正吃进肚子
            if (playerRect.intersects(cakeRect)) {
                // 赋予卡比 20 秒（1200帧）的攻击能力时效！
                player->attackPowerTimer = 1200;

                // 从场景中彻底移除并销毁内存
                scene->removeItem(cake);
                cakes.removeAt(j);
                delete cake;

                // 【核心机制】：蛋糕是直接消化的营养品，不进入Fatty状态
                player->isFatty = false;
                player->endSwallow(); // 直接结束吸气，恢复常规状态以释放光球
                break; // 单帧内只吞噬一个蛋糕
            }
            // 【蛋糕阶段二】：蛋糕处于吸气范围内 -> 产生无法反抗的拉扯飞行效果
            else if (swallowWindRect.intersects(cakeRect)) {
                double pullSpeed = 5.5; // 与吸怪速度保持一致，形成统一的黑洞吸力感
                if (player->facingRight) {
                    cake->setPos(cake->x() - pullSpeed, cake->y());
                } else {
                    cake->setPos(cake->x() + pullSpeed, cake->y());
                }
            }
        }
    }
    // ====== 新增：监听卡比吐出动画的触发帧，生成对应的星星子弹 ======
    if (player->triggerSpitStar) {
        player->triggerSpitStar = false; // 消费信号，立即复位

        // 1. 创建高伤害、高速度的星星子弹
        Projectile* spitStar = new Projectile(player->facingRight);
        spitStar->damage = 3;
        spitStar->vx = player->facingRight ? 16 : -16;

        // 2. 根据肚子里怪物的能力，绘制不同颜色的星星
        QPixmap starPix(32, 32);
        starPix.fill(Qt::transparent);
        QPainter painter(&starPix);

        if (player->swallowedAbility == Enemy::FIRE) {
            painter.setBrush(Qt::red);   // 如果是火系怪，喷出火红的伤害星
        } else {
            painter.setBrush(Qt::cyan);  // 普通怪喷出青色星
        }
        painter.setPen(Qt::NoPen);

        // 绘制钻石交叉星几何
        QPolygonF diamond;
        diamond << QPointF(16, 0) << QPointF(32, 16) << QPointF(16, 32) << QPointF(0, 16);
        painter.drawPolygon(diamond);
        painter.end();
        spitStar->setPixmap(starPix);

        // 3. 将星星精准定位在卡比的嘴部前方
        double startX = player->facingRight ? player->x() + 48 : player->x() - 32;
        double startY = player->y() + 8;
        spitStar->setPos(startX, startY);

        scene->addItem(spitStar);
        projectiles.append(spitStar);

        // 4. 发射完毕，彻底清洗肚子里的复制能力缓存
        player->swallowedAbility = Enemy::NONE;
    }
    // ====== 新增：吃蛋糕碰撞判定与计时器赋予 ======
    QRectF playerRect(player->x(), player->y(), 48, 48); // 抓取卡比包围盒

    // ====== 修改后：给蛋糕加上重力与地面碰撞阻挡 ======
    for (int i = cakes.size() - 1; i >= 0; i--) {
        Cake* cake = cakes[i];
        cake->updateLogic(); // 刷新可能存在的每帧逻辑

        // 1. 应用重力 (采用与敌人相同的自由落体算法)
        cake->vy += 0.8;
        if (cake->vy > 15) cake->vy = 15; // 终端垂直下落速度限制

        // 2. 垂直方向位移
        cake->setPos(cake->x(), cake->y() + cake->vy);

        // 3. 地板碰撞检测：防止蛋糕穿模掉出地图
        QRectF cRect = cake->sceneBoundingRect();
        for (Tile *tile : floors) {
            if (cake->collidesWithItem(tile)) {
                QRectF tRect = tile->sceneBoundingRect();
                if (cake->vy > 0) { // 只有在向下掉落时踩到地板才触发阻挡
                    // 将蛋糕的底部精准贴在方块的上边缘
                    cake->setPos(cake->x(), tRect.top() - cRect.height());
                    cake->vy = 0; // 落地后垂直速度清零，停止下落
                }
                break; // 踩到一块地板后就跳出当前的碰撞检查
            }
        }
    }
    // ====== 闪电形态：生成与更新尾气系统 ======
    // 保险机制：如果受到攻击丢失了形态或按T取消了形态，强制坠机
    if (player->currentForm != Enemy::SPARK) player->isLightningFlying = false;

    if (player->isLightningFlying) {
        // 只有在发生移动时，才喷射尾气
        if (player->vx != 0 || player->vy != 0) {
            // 每 3 帧生成一团尾气
            if (aiTimer % 3 == 0 && !weiqiFrames.isEmpty()) {
                QGraphicsPixmapItem* ex = new QGraphicsPixmapItem();
                int frameIdx = (aiTimer / 3) % weiqiFrames.size();
                QPixmap img = weiqiFrames[frameIdx];

                if (!player->facingRight) img = img.transformed(QTransform().scale(-1, 1));
                ex->setPixmap(img);

                // 定位在卡比屁股后面
                double exX = player->facingRight ? player->x() - img.width() + 10 : player->x() + 48 - 10;
                double exY = player->y() + 24 - img.height() / 2.0;
                ex->setPos(exX, exY);
                ex->setZValue(player->zValue() - 1); // 确保尾气在卡比身体后方

                scene->addItem(ex);
                exhaustItems.append(ex);
                exhaustLifetimes.append(15); // 尾气存活 15 帧
            }
        }
    }

    // 渐变消散尾气
    for (int i = exhaustItems.size() - 1; i >= 0; i--) {
        exhaustLifetimes[i]--;
        exhaustItems[i]->setOpacity(exhaustLifetimes[i] / 15.0); // 随着时间渐渐变透明
        if (exhaustLifetimes[i] <= 0) {
            scene->removeItem(exhaustItems[i]);
            delete exhaustItems[i];
            exhaustItems.removeAt(i);
            exhaustLifetimes.removeAt(i);
        }
    }
}