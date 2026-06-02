#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "basicenemy.h"
#include "apple.h"
#include "xuehua.h"
#include <QPainter>
#include <QCursor>

#include <QStyleFactory>
#include <QMouseEvent>
#include <cmath>
int originalSize = 24;
int renderSize = originalSize * 2;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    player = nullptr;

    // 在 MainWindow 构造函数中，找到 this->resize(1000, 700); 并替换为以下两行：
    this->setFixedSize(1000, 700);
    this->setWindowFlags(Qt::Window | Qt::MSWindowsFixedSizeDialogHint);
    // 1. 场景
    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 52000, 1200); // 地图长度设为 52000
    scene->setBackgroundBrush(QColor(135, 206, 235)); // 默认天空蓝背景

    view = new QGraphicsView(scene, this);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 固定高度通常不需要垂直滚动条
    view->setMouseTracking(true);
    view->viewport()->setMouseTracking(true);
    view->installEventFilter(this);
    setCentralWidget(view);

    qreal sceneH = scene->sceneRect().height();
    // 四层天空背景（修改部分：记录到 backgroundLayers）
    QStringList bgPaths = {
        ":/tu/Clouds 1/1.png",
        ":/tu/Clouds 1/2.png",
        ":/tu/Clouds 1/3.png",
        ":/tu/Clouds 1/4.png"
    };

    // 切割方块素材
    QPixmap tileSheet(":/tu/fangkuai.png");
    grass        = tileSheet.copy(0, 0, originalSize, originalSize);
    dirt         = tileSheet.copy(originalSize * 1, 0, originalSize, originalSize);
    waterSurface = tileSheet.copy(originalSize * 2, 0, originalSize, originalSize);
    QPixmap waterBody    = tileSheet.copy(originalSize * 3, 0, originalSize, originalSize);


    // 加载冰块和碎石的素材
    QPixmap iceBlock(":/tu/ice.png");
    QPixmap rubble(":/tu/suishi.png");
    // 加载五种刺的素材
    QPixmap ci(":/tu/ci.png");
    QPixmap ci2(":/tu/ci2.png");
    QPixmap ci3(":/tu/ci3.png");
    QPixmap qiangci(":/tu/qiangci.png");
    QPixmap daoci(":/tu/daoci.png");
    // ====== 新增：切割尾气素材（假设为横向两帧） ======
    QPixmap weiqiSheet(":/tu/weiqi.png");

    rubblePix = rubble.copy(0, 0, originalSize, originalSize);
    ciPix = ci.copy(originalSize - 7, originalSize * 2 + 3, originalSize, originalSize * 2);
    ci2Pix = ci2.copy(originalSize / 2.0, 0, originalSize, originalSize);
    ci3Pix = ci3.copy(originalSize - 5, originalSize / 4.0 + 4, originalSize, originalSize);
    qiangciPix = qiangci.copy(originalSize / 2.0, originalSize / 2.0, originalSize, originalSize * 4);
    daociPix = daoci.copy(originalSize / 2.0, originalSize / 2.0, originalSize * 4, originalSize);
    waterBodyPix = tileSheet.copy(originalSize * 3, 0, originalSize, originalSize);
    iceBlockPix = iceBlock.copy(0, 0, originalSize, originalSize);

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

    // ====== 预加载猪鲨二阶段背景（Clouds 3），初始透明 ======
    QStringList bgPathsPhase2 = {
        ":/tu/Clouds 3/1.png",
        ":/tu/Clouds 3/2.png",
        ":/tu/Clouds 3/3.png",
        ":/tu/Clouds 3/4.png"
    };
    for (int i = 0; i < bgPathsPhase2.size(); ++i) {
        QPixmap pix(bgPathsPhase2[i]);
        if (!pix.isNull()) {
            QPixmap scaledPix = pix.scaledToHeight(sceneH, Qt::SmoothTransformation);
            QGraphicsRectItem* bg = new QGraphicsRectItem(0, 0, scaledPix.width(), sceneH);
            bg->setBrush(QBrush(scaledPix));
            bg->setPen(Qt::NoPen);
            bg->setZValue(-96 + i); // 在Clouds 1（-100~-97）之上
            bg->setOpacity(0.0);
            scene->addItem(bg);
            phase2BgLayers.append(bg);
        }
    }

    if (!weiqiSheet.isNull()) {
        int count = 2;
        int fw = weiqiSheet.width() / count;
        int fh = weiqiSheet.height();
        for (int i = 0; i < count; i++) {
            weiqiFrames.push_back(weiqiSheet.copy(i * fw, 0, fw, fh));
        }
    }

    // ====== 初始化体力条 ======
    staminaBar = new QProgressBar(this);
    staminaBar->setRange(0, 300);
    staminaBar->setValue(300);
    staminaBar->setTextVisible(false);
    staminaBar->setVisible(false);
    staminaBar->setGeometry(20, 75, 175, 12);
    staminaBar->setStyle(QStyleFactory::create("Fusion"));

    // 使用 QSS 样式表美化：深灰色背景 + 亮蓝色进度条
    staminaBar->setStyleSheet(
        "QProgressBar {"
        "   border: 2px solid #333;"
        "   border-radius: 3px;"
        "   background-color: #222;" /* 没体力时的黑色背景 */
        "}"
        "QProgressBar::chunk {"
        "   background-color: #00aaff;" /* 有体力时的纯蓝色块，无渐变 */
        "}"
        );

    // 4. 关卡矩阵 (保持不变，确保地图长度足够)
    QStringList levelData = {
        ".......................",
        ".......................",
        ".......................",
        ".......................",
        ".......................",
        ".......................",
        ".......................",
        ".......................",
        ".......................",
        "11111111111111111111111",
        "22222222222222222222222",

    };

    int totalMapHeight = levelData.size() * renderSize;
    int bottomOffset = sceneH - totalMapHeight;

    for (int r = 0; r < levelData.size(); r++) {
        for (int c = 0; c < levelData[r].length(); c++) {
            Tile *tile = nullptr;
            char type = levelData[r][c].toLatin1();
            if (type == '1')  tile = new Tile(Tile::Grass, grass);
            else if (type == '2')  tile = new Tile(Tile::Dirt, dirt);
            else if (type == '3')  tile = new Tile(Tile::WaterSurface, waterSurface);
            else if (type == '4')  tile = new Tile(Tile::WaterBody, waterBody);
            else if (type == '5')  tile = new Tile(Tile::IceBlock, iceBlockPix);
            if (tile) {
                tile->setPos(c * renderSize, r * renderSize + bottomOffset);
                tile->setScale(2.0);
                scene->addItem(tile);
                if (type == '1' || type == '2' || type == '5' || type == '6') floors.append(tile);
                else if (type == '3' || type == '4') waters.append(tile);
            }
        }
    }

    // 5. 卡比
    player = new Player();
    player->setPos(800, 856);
    scene->addItem(player);

    // Boss血条（始终存在，跨关卡复用）
    bossHpBarBg = new QGraphicsRectItem(0, 0, 200, 10);
    bossHpBarBg->setBrush(QBrush(QColor(60, 60, 60)));
    bossHpBarBg->setPen(Qt::NoPen);
    bossHpBarBg->setZValue(2000);
    bossHpBarBg->setVisible(false);
    scene->addItem(bossHpBarBg);

    bossHpBarFg = new QGraphicsRectItem(0, 0, 200, 10);
    bossHpBarFg->setBrush(QBrush(QColor(220, 30, 30)));
    bossHpBarFg->setPen(Qt::NoPen);
    bossHpBarFg->setZValue(2001);
    bossHpBarFg->setVisible(false);
    scene->addItem(bossHpBarFg);

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

    // ====== 形态技能冷却图标 (左下角) ======
    auto createCdIcon = [&](const QString &path) -> QGraphicsPixmapItem* {
        QPixmap pix(path);
        QGraphicsPixmapItem* item = new QGraphicsPixmapItem(pix);
        if (!pix.isNull()) {
            item->setScale(1.2);
        }
        item->setZValue(2000);
        item->setVisible(false);
        scene->addItem(item);
        return item;
    };
    fireCdIcon = createCdIcon(":/tu/fire_lenque.png");
    iceCdIcon  = createCdIcon(":/tu/ice_lenque.png");
    leafCdIcon = createCdIcon(":/tu/leaf_lenque.png");

    // 初始复活位置 = 卡比出生点
    lastCheckpointPos = QPointF(800, 856);

    // 音乐
    bgmPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    bgmPlayer->setAudioOutput(audioOutput);
    originalBgmSource = "qrc:///tu/kerby theme music.mp3";
    bgmPlayer->setSource(QUrl(originalBgmSource));
    audioOutput->setVolume(volumeLevel); // 音量范围 0.0 到 1.0
    bgmPlayer->setLoops(QMediaPlayer::Infinite); // 无限循环
    bgmPlayer->play();

    // ====== 音效系统初始化 ======
    stepPlayer = new QMediaPlayer(this);
    stepAudio = new QAudioOutput(this);
    stepPlayer->setAudioOutput(stepAudio);
    stepPlayer->setSource(QUrl("qrc:///tu/music/foley_footstep_concrete_1.wav"));
    stepAudio->setVolume(0.3);
    stepPlayer->setLoops(QMediaPlayer::Infinite);

    waterPlayer = new QMediaPlayer(this);
    waterAudio = new QAudioOutput(this);
    waterPlayer->setAudioOutput(waterAudio);
    waterPlayer->setSource(QUrl("qrc:///tu/music/water_babbling_loop.wav"));
    waterAudio->setVolume(0.3);
    waterPlayer->setLoops(QMediaPlayer::Infinite);

    sfxPlayer = new QMediaPlayer(this);
    sfxAudio = new QAudioOutput(this);
    sfxPlayer->setAudioOutput(sfxAudio);
    sfxAudio->setVolume(0.5);

    cinematicPlayer = new QMediaPlayer(this);
    cinematicAudio = new QAudioOutput(this);
    cinematicPlayer->setAudioOutput(cinematicAudio);
    cinematicAudio->setVolume(0.5);

    // Boss登场音效
    bossStingerPlayer = new QMediaPlayer(this);
    bossStingerAudio = new QAudioOutput(this);
    bossStingerPlayer->setAudioOutput(bossStingerAudio);

    // RPG战斗音乐列表
    bossMusicTracks = {
        "qrc:///tu/music/rpg_bs004.m4a",
        "qrc:///tu/music/rpg_bs005.m4a",
        "qrc:///tu/music/rpg_bs008.m4a",
        "qrc:///tu/music/rpg_bs009.m4a",
        "qrc:///tu/music/rpg_bs010.m4a",
    };

    // 6. 游戏循环
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::gameUpdate);
    timer->start(16);

}
MainWindow::~MainWindow() { delete ui; }

void MainWindow::loadLevel(int levelNum) {
    currentLevelNum = levelNum;
    // ================== 1. 安全清理上一关残留的物体 ==================

    // A. 清理怪物
    for (Enemy* t : enemies) { scene->removeItem(t); delete t; }
    enemies.clear();

    // B. 清理水体
    for (Tile* t : waters) { scene->removeItem(t); delete t; }
    waters.clear();

    // C. 清理地板（注意：这里已经把普通方块和地刺全部 delete 销毁了！）
    for (Tile* t : floors) { scene->removeItem(t); delete t; }
    floors.clear();

    // D. 【核心修正】：因为地刺在上面 floors 循环里已经被销毁了，
    // 这里绝对不能再 delete t！只需要清空指针列表即可！
    spikes.clear();

    // E. 清理上关遗留的蛋糕和星星（防止遗漏内存泄漏）
    for (Cake* c : cakes) { scene->removeItem(c); delete c; }
    cakes.clear();
    for (Star* s : stars) { scene->removeItem(s); delete s; }
    stars.clear();
    bossStarSpawnTimer = 0;

    // F. 清理上关遗留的检查点（防止遗漏内存泄漏）
    for (Checkpoint* cp : checkpoints) { scene->removeItem(cp); delete cp; }
    checkpoints.clear();

    // G. 清理上关遗留的终点
    for (Goal* g : goals) { scene->removeItem(g); delete g; }
    goals.clear();

    // G2. 清理上关遗留的木箱
    for (Crate* crate : crates) { scene->removeItem(crate); delete crate; }
    crates.clear();

    // H. 安全删除玩家
    if (player != nullptr) {
        scene->removeItem(player);
        delete player;
        player = nullptr; // 销毁后立刻置空
    }

    if (!scene) return; // 安全检查
    qreal sceneH = scene->sceneRect().height();

    // 2. 根据关卡数，选择不同的矩阵
    QStringList levelData;
    if (levelNum == 1) {
        levelData = {
            "........................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................",
            "........................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................",
            "........................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................",
            "........................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................",
            "........................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................",
            "........................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................",
            "........................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................",
            "..............................1111..........................................................................................................................................................................................................................................................................................................1111........................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................................",
            "...........................................................................................................................1111......................................................................................................................................................................................................................................................................................111...................................................................................................................................................................................................................................1111.........................................................................................................................................................................................................................................................................................................................................................................",
            "..1111................................................................111..................................................................................111................................................................5555....................................1111..................................................................................................................................................................................................................................................................................................................................111.......1111.................................................................................................................111C1111...........................11C111..................................................................................................................................................................................................11C11.............................................",
            "........................................1111................................................................1111............................................................................44444444...............................................................................................................................................................111.....................................111............................................................................................1111....................................................................................................................................................................................................1111..............................................................................C.111...............................................................................................................................................................................................................................",
            "...............111.....................................111111..................................111.....................................111................................1111..............44444444..............1111.............................................................111....................................................111.............................................................................................................................111.............................................................................................................1111............................................1111........................................1111................................C...1111..................................1111..111...................111..................111............................1111......................................111C11........................1111.......................................111C1..................1111......................................",
            "........................................................................................................................1111................................................................33333333.......................................111............5555................................................1111................................................................1111..1......................................111..........666..............666.................666.......111..................................111.................1111...................G..111..1111.G...........11111.......G.............111......G..........................G..1111...........................................................111.L...............L.11111.......................L...111.............L.111111................111......111.E........................1111..............1.............111.............G.....................................111.............1111......................1111......1111...TTT............",
            "P1111111E1E111111111C1111E1E1111111C111111111E1E11C11111111111I11C111111111K11C111111E1E1111F1111111I1111K111111111E1E1111111111F111C1111111I1111K11555511111111C1111E1E1111111111P111I11K11........1111555111111111111111I11111111111K111111111C1111E1E1111111111I11111111111K111111111C1111E1E1155551111I11111111111C1111F11P155555E555555555555C555555E5E55I55555555511111K111111111..11155555C55E51111111111K111111111C1111...P111F11111666111C11111111116661111C11F111111111666111C111...11F1111E1E1111111111C111666111F111111111K111555111C111....1111F1111P1111111111111111....111111111111C1.....11111111111111111C11.....111111111111111C11E1E111111111111111111111....11C1I111111111P11111111111111L11.......11111....11....111111111111....111111111111.......111111111111111........11111111111111111111....111K11P111...111F1....1111I111111..1C1L1111K1111....1F11C155E55511..G111111111......11I1K1E11F1111C1....1111L1111P11111111111111C11C1.....11C111111111....11C111111111....111C111111111C11....11111111C1111C11C1",
            "2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222",
        };
    } else if (levelNum == 99) {
        // Boss战：平地地图（80列，两边有墙）
        int bossRows = 10;
        int bossCols = 80;
        int bossBottomOffset = sceneH - bossRows * renderSize;
        levelData = {
            "                                                                                ",
            "                                                                                ",
            "                                                                                ",
            "                                                                                ",
            "                                                                                ",
            "                                                                                ",
            "                                                                                ",
            "                                                                                ",
            "1111111111111111111111111111111111111111111111111111111111111111111111111111111111",
            "2222222222222222222222222222222222222222222222222222222222222222222222222222222222",
        };
        // 在地图两侧叠加透明方块形成空气墙（防止卡比走出地图）
        // 从场景顶部以下一直延伸到场景底部
        {
            QPixmap transparentPix(renderSize, renderSize);
            transparentPix.fill(Qt::transparent);
            int wallRows = 25;  // 1200px 覆盖整个场景高度
            for (int r = -4; r < wallRows; r++) {
                for (int c = 0; c < 2; c++) {
                    Tile* wall = new Tile(Tile::Dirt, transparentPix);
                    wall->setPos(c * renderSize, r * renderSize + bossBottomOffset);
                    scene->addItem(wall);
                    floors.append(wall);
                }
                for (int c = bossCols - 2; c < bossCols; c++) {
                    Tile* wall = new Tile(Tile::Dirt, transparentPix);
                    wall->setPos(c * renderSize, r * renderSize + bossBottomOffset);
                    scene->addItem(wall);
                    floors.append(wall);
                }
            }
        }
    }

    int totalMapHeight = levelData.size() * renderSize;
    int bottomOffset = sceneH - totalMapHeight;

    for (int r = 0; r < levelData.size(); r++) {
        for (int c = 0; c < levelData[r].length(); c++) {
            Tile *tile = nullptr;
            char type = levelData[r][c].toLatin1();
            if (type == 'I') {
                MinionEnemy* iceEnemy = new MinionEnemy(":/tu/Ice_Dude.png", 6, 1.2, Enemy::ICE);
                iceEnemy->setScale(0.6);
                iceEnemy->setPos(c * renderSize, r * renderSize + bottomOffset);
                //iceEnemy->setVisible(false);
                scene->addItem(iceEnemy);
                enemies.append(iceEnemy);
                continue;
            }

            // ====== 2. 【新增】处理带火能力的小怪物 F ======
            else if (type == 'F') {
                MinionEnemy* fireEnemy = new MinionEnemy(":/tu/fire_enemy.png", 5, 1.5, Enemy::FIRE);
                fireEnemy->setScale(2);
                fireEnemy->setPos(c * renderSize, r * renderSize + bottomOffset);
                //fireEnemy->setVisible(false);
                scene->addItem(fireEnemy);
                enemies.append(fireEnemy);
                continue;
            }
            else if (type == 'G'){
                MinionEnemy* leafEnemy = new MinionEnemy(":/tu/Leaf_Dude.png", 8, 1.0, Enemy::LEAF);
                leafEnemy->setScale(0.6);
                leafEnemy->setPos(c * renderSize, r * renderSize + bottomOffset);
                //leafEnemy->setVisible(false);
                scene->addItem(leafEnemy);
                enemies.append(leafEnemy);
                continue;
            }
            else if (type == 'L'){
                MinionEnemy* lightningEnemy = new MinionEnemy(":/tu/Lightning_Dude.png", 6, 1.8, Enemy::SPARK);
                lightningEnemy->setScale(0.6);
                lightningEnemy->setPos(c * renderSize, r * renderSize + bottomOffset);
                //lightningEnemy->setVisible(false);
                scene->addItem(lightningEnemy);
                enemies.append(lightningEnemy);
                continue;
            }
            else if (type == 'E') {
                MinionEnemy* basicEnemy = new MinionEnemy(":/tu/basic_enemy.png", 3, 1.0, Enemy::NONE);
                basicEnemy->setScale(2.0);
                basicEnemy->setPos(c * renderSize, r * renderSize + bottomOffset);
                scene->addItem(basicEnemy);
                enemies.append(basicEnemy);
                continue;
            }
            else if (type == '1')  tile = new Tile(Tile::Grass, grass);
            else if (type == '2')  tile = new Tile(Tile::Dirt, dirt);
            else if (type == '3')  tile = new Tile(Tile::WaterSurface, waterSurface);
            else if (type == '4')  tile = new Tile(Tile::WaterBody, waterBodyPix);
            else if (type == '5')  tile = new Tile(Tile::IceBlock, iceBlockPix);
            else if (type == '6')  tile = new Tile(Tile::RubbleBlock, rubblePix);
            else if (type == '7')  tile = new Tile(Tile::Spike, ciPix);
            else if (type == '8')  tile = new Tile(Tile::Spike, ci2Pix);
            else if (type == '9')  tile = new Tile(Tile::Spike, ci3Pix);
            else if (type == 'A')  tile = new Tile(Tile::Spike, qiangciPix);
            else if (type == 'B')  tile = new Tile(Tile::Spike, daociPix);
            else if (type == 'C'){
                Cake* cake = new Cake();
                cake->setPos(c * renderSize, r * renderSize + bottomOffset);
                //cake->setVisible(false);
                scene->addItem(cake);
                cakes.append(cake);
            }
            else if (type == 'K') {
                Crate* crate = new Crate();
                crate->setPos(c * renderSize, r * renderSize + bottomOffset);
                scene->addItem(crate);
                crates.append(crate);
            }
            else if (type == 'P') {
                Checkpoint* cp = new Checkpoint();
                // 采用跟你地形一模一样的坐标计算公式和缩放
                cp->setPos(c * renderSize, r * renderSize + bottomOffset - 30);
                cp->setScale(2.0);
                //cp->setVisible(false);
                scene->addItem(cp);
                checkpoints.append(cp); // 塞进主循环检测的检查点列表
                continue; // 检查点处理完，直接跳过下面的 tile 判定
            }
            else if (type == 'T') {
                Goal* goal = new Goal();
                goal->setPos(c * renderSize, r * renderSize + bottomOffset - 48);
                scene->addItem(goal);
                goals.append(goal);
                continue;
            }
            if (tile) {
                tile->setPos(c * renderSize, r * renderSize + bottomOffset);
                tile->setScale(2.0);
                scene->addItem(tile);
                if (type == '1' || type == '2' || type == '5' || type == '6') floors.append(tile);
                else if (type == '3' || type == '4') waters.append(tile);
                if (type == '7' || type == '8' || type == '9' || type == 'A' || type == 'B') {
                    floors.append(tile);  // 刺也能站上去（物理碰撞）
                    spikes.append(tile);  // 但对玩家造成伤害
                }
            }
        }
    }

    // ====== 3.5 自动填充地面坑洞 ======
    {
        int groundRowIdx = levelData.size() - 2;
        if (groundRowIdx >= 0) {
            for (int c = 0; c < levelData[groundRowIdx].length(); c++) {
                if (levelData[groundRowIdx][c] == '.') {
                    // 确认下方是泥土（即确实是地面，而非浮空平台）
                    if (levelData.size() > groundRowIdx + 1 && levelData[groundRowIdx + 1][c] == '2') {
                        Tile* fillTile = new Tile(Tile::Grass, grass);
                        fillTile->setPos(c * renderSize, groundRowIdx * renderSize + bottomOffset);
                        fillTile->setScale(2.0);
                        scene->addItem(fillTile);
                        floors.append(fillTile);
                    }
                }
            }
        }
    }

    // ====== 3.6 补充额外存档点（每约200列一个） ======
    {
        int groundRowIdx = levelData.size() - 2;
        QVector<int> extraCpCols = { 200, 400, 600, 800, 900 };
        for (int col : extraCpCols) {
            if (col < levelData[groundRowIdx].length()) {
                Checkpoint* cp = new Checkpoint();
                cp->setPos(col * renderSize, groundRowIdx * renderSize + bottomOffset - 30);
                cp->setScale(2.0);
                scene->addItem(cp);
                checkpoints.append(cp);
            }
        }
    }

    // 4. 重生卡比
    player = new Player();
    player->setPos(800, 856);
    scene->addItem(player);
    lastCheckpointPos = QPointF(800, 856);
    // 5. 切换 UI 状态 (隐藏菜单字，显示 HUD)
    staminaBar->setVisible(true);
    for (auto icon : lifeIcons) icon->setVisible(true);

    // 5.5 浮动文字教程初始化
    tutorialTriggers.clear();
    tutorialTriggers = {
        {1300, QStringLiteral("WASD / 方向键 - 移动，K - 跳跃 / 二段跳")},
        {2500, QStringLiteral("J - 攻击！吃蛋糕获得攻击力")},
        {4500, QStringLiteral("L - 吸入敌人，再按L消化获得能力！")},
        {7000, QStringLiteral("普通敌人不可吞噬，用攻击或踩踏消灭")},
        {10000, QStringLiteral("冰敌(I)可获得冰形态，按J冻结水面！")},
        {12000, QStringLiteral("木箱可推动，填河或垫脚过障碍")},
        {15500, QStringLiteral("火敌(F)可获得火形态，按J冲刺爆破！")},
        {19000, QStringLiteral("叶敌(G)远程攻击，保持距离！")},
        {23000, QStringLiteral("电敌(L)可飞行，探索隐藏区域！")},
        {29000, QStringLiteral("灵活切换能力应对不同地形！")},
        {35000, QStringLiteral("前方混合区，小心应对！")},
        {41000, QStringLiteral("终点就在前方！加油！")},
    };

    // 6. 状态机切换与启动
    currentState = PLAYING;
    timer->start(16);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    // ====== ESC暂停/返回 ======
    if (event->key() == Qt::Key_Escape && !event->isAutoRepeat()) {
        if (currentState == LevelSelect) {
            cleanupSelectUI();
            showMainMenu();
            return;
        }
        if (currentState == PLAYING) {
            savedLevelNum = currentLevelNum;
            savedCheckpointPos = lastCheckpointPos;
            savedForm = player->currentForm;
            savedHP = player->hp;
            savedStamina = player->stamina;
            savedAttackPowerTimer = player->attackPowerTimer;

            qreal camX = player->x();
            qreal camY = 850;

            pauseOverlay = new QGraphicsRectItem(camX - SCREEN_W/2.0, camY - SCREEN_H/2.0, SCREEN_W, SCREEN_H);
            pauseOverlay->setBrush(QBrush(QColor(0, 0, 0, 170)));
            pauseOverlay->setPen(Qt::NoPen);
            pauseOverlay->setZValue(2000);
            scene->addItem(pauseOverlay);

            pauseTitle = new QGraphicsTextItem("游戏暂停");
            pauseTitle->setFont(QFont("SimHei", 40, QFont::Bold));
            pauseTitle->setDefaultTextColor(Qt::white);
            pauseTitle->setZValue(2001);
            double tw = pauseTitle->boundingRect().width();
            pauseTitle->setPos(camX - tw/2.0, camY - SCREEN_H/3.0);
            scene->addItem(pauseTitle);

            qreal btnW = 260, btnH = 50;
            qreal btnX = camX - btnW/2.0;
            qreal btnY = camY - SCREEN_H/6.0;

            // 继续游戏按钮
            QRectF contRect(btnX, btnY, btnW, btnH);
            pauseBtnRects.append(contRect);
            QGraphicsRectItem* cb = new QGraphicsRectItem(contRect);
            cb->setBrush(QBrush(QColor(30, 80, 30, 200)));
            cb->setPen(QPen(QColor(100, 200, 100), 2));
            cb->setZValue(2001);
            scene->addItem(cb);
            pauseButtons.append(cb);
            QGraphicsTextItem* ct = new QGraphicsTextItem("继续游戏");
            ct->setFont(QFont("SimHei", 18, QFont::Bold));
            ct->setDefaultTextColor(Qt::white);
            ct->setZValue(2002);
            ct->setPos(btnX + 60, btnY + 10);
            scene->addItem(ct);
            pauseButtonTexts.append(ct);

            // 音量滑块
            btnY += btnH + 30;
            qreal trackW = btnW, trackH = 20;
            volTrackRect = QRectF(btnX, btnY, trackW, trackH);
            volTrackBg = new QGraphicsRectItem(volTrackRect);
            volTrackBg->setBrush(QBrush(QColor(60, 60, 60)));
            volTrackBg->setPen(QPen(QColor(120, 120, 120), 1));
            volTrackBg->setZValue(2001);
            scene->addItem(volTrackBg);
            volTrackFg = new QGraphicsRectItem(btnX, btnY, trackW * volumeLevel, trackH);
            volTrackFg->setBrush(QBrush(QColor(80, 160, 255)));
            volTrackFg->setPen(Qt::NoPen);
            volTrackFg->setZValue(2002);
            scene->addItem(volTrackFg);
            volHandle = new QGraphicsRectItem(btnX + trackW * volumeLevel - 4, btnY - 4, 12, trackH + 8);
            volHandle->setBrush(QBrush(QColor(220, 220, 220)));
            volHandle->setPen(QPen(Qt::white, 1));
            volHandle->setZValue(2003);
            scene->addItem(volHandle);

            QGraphicsTextItem* vl = new QGraphicsTextItem("音量");
            vl->setFont(QFont("SimHei", 14));
            vl->setDefaultTextColor(QColor(180, 180, 180));
            vl->setZValue(2002);
            vl->setPos(btnX, btnY - 26);
            scene->addItem(vl);
            pauseButtonTexts.append(vl);

            // 退出按钮
            btnY += trackH + 30;
            QRectF exitRect(btnX, btnY, btnW, btnH);
            pauseBtnRects.append(exitRect);
            QGraphicsRectItem* eb = new QGraphicsRectItem(exitRect);
            eb->setBrush(QBrush(QColor(80, 30, 30, 200)));
            eb->setPen(QPen(QColor(200, 100, 100), 2));
            eb->setZValue(2001);
            scene->addItem(eb);
            pauseButtons.append(eb);
            QGraphicsTextItem* et = new QGraphicsTextItem("退出至主菜单");
            et->setFont(QFont("SimHei", 18, QFont::Bold));
            et->setDefaultTextColor(Qt::white);
            et->setZValue(2002);
            et->setPos(btnX + 50, btnY + 10);
            scene->addItem(et);
            pauseButtonTexts.append(et);

            currentState = PAUSED;
            return;
        }
    }

    if (event->isAutoRepeat()) return;
    if (!player) return;
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

    // 只在游戏和选关状态下记录按键，防止菜单状态下按键泄露
    if (currentState == PLAYING || currentState == LevelSelect)
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
            // 星星单次攻击优先（吞噬星星获得的特殊攻击）
            if (player->starAttackStock > 0) {
                player->starAttackStock--;
                player->startAttack();
                // 星星攻击音效
                sfxPlayer->setSource(QUrl("qrc:///tu/music/snap.wav"));
                sfxAudio->setVolume(0.5);
                sfxPlayer->play();
                // 生成星星子弹（使用星星贴图）
                Projectile* proj = new Projectile(player->facingRight);
                proj->damage = 50;
                proj->vx = player->facingRight ? 16 : -16;
                proj->setPixmap(QPixmap(":/tu/star.png").scaled(32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                double startX = player->facingRight ? player->x() + 48 : player->x() - 24;
                double startY = player->y() + 12;
                proj->setPos(startX, startY);
                scene->addItem(proj);
                projectiles.append(proj);
            } else if (player->hasAttackPower()) {
                player->startAttack();
                // 普通攻击音效
                sfxPlayer->setSource(QUrl("qrc:///tu/music/snap.wav"));
                sfxAudio->setVolume(0.5);
                sfxPlayer->play();
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
                // 叶形态攻击音效
                sfxPlayer->setSource(QUrl("qrc:///tu/music/snap.wav"));
                sfxAudio->setVolume(0.5);
                sfxPlayer->play();
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
                player->endLightningDash(); // 关掉飞行时结束冲刺
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
    // ====== U键发射星星（消耗starAttackStock，所有形态通用） ======
    else if (event->key() == Qt::Key_U) {
        if (player->starAttackStock <= 0) return;
        if (player->isRolling || player->isSwallowing || player->isSpitting ||
            player->isFireSprinting || player->isExploding || player->isDigesting) return;

        player->starAttackStock--;
        sfxPlayer->setSource(QUrl("qrc:///tu/music/snap.wav"));
        sfxAudio->setVolume(0.5);
        sfxPlayer->play();
        Projectile* proj = new Projectile(player->facingRight);
        proj->damage = 50;
        proj->vx = player->facingRight ? 16 : -16;
        proj->setPixmap(QPixmap(":/tu/star.png").scaled(32, 32, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        double startX = player->facingRight ? player->x() + 48 : player->x() - 24;
        double startY = player->y() + 12;
        proj->setPos(startX, startY);
        scene->addItem(proj);
        projectiles.append(proj);
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
            // 2. 构造一个探测矩形：左右各扩展 3 个方块，上下扩展半个方块（限定同一高度）
            int range = 3 * renderSize;
            QRectF freezeBox = player->sceneBoundingRect().adjusted(-range, -renderSize/2, range, renderSize/2);

            // 3. 获取在这个区域内的所有物品
            QList<QGraphicsItem*> itemsInBox = scene->items(freezeBox);

            for (QGraphicsItem* item : itemsInBox) {
                Tile* tile = dynamic_cast<Tile*>(item);
                if (tile && (tile->tileType() == Tile::WaterSurface || tile->tileType() == Tile::WaterBody)) {

                    // 严格过滤：只冻结与卡比几乎在同一高度的水
                    if (abs(tile->y() - player->y()) - 10 < renderSize) {
                        // 将水变成冰块！
                        tile->changeType(Tile::IceBlock, iceBlockPix); // 调用你现有的切换材质函数

                        // 关键点：如果是新生成的实体冰块，记得加入碰撞地板列表
                        if (!floors.contains(tile)) {
                            floors.append(tile);
                        }
                    }
                }
            }
        }
    } else if (!keys.contains(Qt::Key_J) && player->isIceDefending) {
        // 松开 J 键时，立刻解除防御状态并开始 10 秒冷却
        player->endIceDefend();
    }

    // ====== 调试热键：F1-F10 跳转到地图不同段 ======
    if (currentState == PLAYING && player) {
        int section = -1;
        if (event->key() == Qt::Key_F1) section = 0;
        else if (event->key() == Qt::Key_F2) section = 1;
        else if (event->key() == Qt::Key_F3) section = 2;
        else if (event->key() == Qt::Key_F4) section = 3;
        else if (event->key() == Qt::Key_F5) section = 4;
        else if (event->key() == Qt::Key_F6) section = 5;
        else if (event->key() == Qt::Key_F7) section = 6;
        else if (event->key() == Qt::Key_F8) section = 7;
        else if (event->key() == Qt::Key_F9) section = 8;
        else if (event->key() == Qt::Key_F10) section = 9;

        if (section >= 0) {
            qreal mapW = scene->sceneRect().width();
            qreal targetX = (mapW / 10.0) * section + 100;
            if (targetX > mapW - 200) targetX = mapW - 200;
            player->setPos(targetX, player->y());
            player->vy = 0;
            player->vx = 0;
        }
    }
}
void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    keys.remove(event->key());
    if (event->key() == lastHorizontalKey)
        lastHorizontalKey = 0;
}

void MainWindow::gameUpdate() {
    // ====== 主菜单延迟初始化（首次进入 MAIN_MENU 时创建UI） ======
    if (currentState == MAIN_MENU && mainMenuButtons.isEmpty()) {
        showMainMenu();
        return;
    }

    // ====== 每帧UI悬停变色（放在最前面，确保在所有 return 前执行） ======
    {
        QPoint vp = view->viewport()->mapFromGlobal(QCursor::pos());
        QPointF sp = view->mapToScene(vp);
        qreal sx = sp.x(), sy = sp.y();

        if (currentState == MAIN_MENU) {
            for (int i = 0; i < mainMenuBtnRects.size() && i < mainMenuButtons.size(); i++) {
                bool in = mainMenuBtnRects[i].contains(sx, sy);
                QColor n, h;
                if (i == 0)                      { n=QColor(30,50,90,210);  h=QColor(50,80,150,230); }
                else if (i == mainMenuBtnCount-1) { n=QColor(80,25,25,210); h=QColor(160,50,50,230); }
                else                              { n=QColor(60,60,60,210); h=QColor(100,100,100,230); }
                mainMenuButtons[i]->setBrush(in ? QBrush(h) : QBrush(n));
            }
        }

        if (currentState == LevelSelect) {
            if (isBossSelect) {
                for (int i = 0; i < bossCardRects.size() && i < bossCards.size(); i++) {
                    bool in = bossCardRects[i].contains(sx, sy);
                    QColor n(50,50,80,200), h(80,80,160,230);
                    if (bossCards[i]) {
                        bossCards[i]->setBrush(in ? QBrush(h) : QBrush(n));
                        bossCards[i]->setPen(QPen(in ? QColor(255,255,150) : QColor(150,150,200), in ? 3 : 2));
                    }
                }
                if (bossBackLabel && bossBackLabel->isVisible()) {
                    QRectF br = bossBackLabel->sceneBoundingRect().adjusted(-10,-5,10,5);
                    bossBackLabel->setDefaultTextColor(br.contains(sx,sy) ? QColor(255,255,255) : QColor(180,180,255));
                }
            } else {
                for (int i = 0; i < cardRects.size() && i < levelCards.size(); i++) {
                    bool in = cardRects[i].contains(sx, sy);
                    QColor n = (i==0)?QColor(30,70,30,210):QColor(70,30,30,210);
                    QColor h = (i==0)?QColor(50,140,50,230):QColor(140,50,50,230);
                    QPen np((i==0)?QColor(100,200,100):QColor(200,80,80), 2);
                    if (levelCards[i]) {
                        levelCards[i]->setBrush(in ? QBrush(h) : QBrush(n));
                        levelCards[i]->setPen(QPen(in ? QColor(255,255,150) : np.color(), in ? 3 : 2));
                    }
                }
            }
        }

        if (currentState == PAUSED) {
            for (int i = 0; i < pauseBtnRects.size() && i < pauseButtons.size(); i++) {
                bool in = pauseBtnRects[i].contains(sx, sy);
                QColor c1 = (i==0)?QColor(30,80,30,200):QColor(80,30,30,200);
                QColor c2 = (i==0)?QColor(50,140,50,230):QColor(140,50,50,230);
                pauseButtons[i]->setBrush(in ? QBrush(c2) : QBrush(c1));
            }
            if (volHandle) {
                bool over = volTrackRect.contains(sx, sy);
                volHandle->setBrush(over ? QBrush(QColor(255,255,255)) : QBrush(QColor(220,220,220)));
                volHandle->setPen(QPen(over ? QColor(255,255,150) : Qt::white, over ? 2 : 1));
            }
        }

        if (currentState == SETTINGS) {
            if (!settingsBtnRects.isEmpty() && !settingsButtons.isEmpty()) {
                bool in = settingsBtnRects[0].contains(sx, sy);
                settingsButtons[0]->setBrush(in ? QBrush(QColor(50,80,150,230)) : QBrush(QColor(30,50,90,210)));
            }
            if (settingsVolHandle && !settingsVolRect.isNull()) {
                bool over = settingsVolRect.contains(sx, sy);
                settingsVolHandle->setBrush(over ? QBrush(QColor(255,255,255)) : QBrush(QColor(220,220,220)));
                settingsVolHandle->setPen(QPen(over ? QColor(255,255,150) : Qt::white, over ? 2 : 1));
            }
        }

        if (currentState == GAME_OVER) {
            for (int i = 0; i < gameOverBtnRects.size() && i < gameOverButtons.size(); i++) {
                bool in = gameOverBtnRects[i].contains(sx, sy);
                QColor c1 = (i==0)?QColor(30,80,30,210):QColor(80,25,25,210);
                QColor c2 = (i==0)?QColor(50,140,50,230):QColor(140,50,50,230);
                gameOverButtons[i]->setBrush(in ? QBrush(c2) : QBrush(c1));
            }
        }

        if (currentState == ENDING && endingMenuBtn) {
            bool in = endingMenuBtn->sceneBoundingRect().contains(sx, sy);
            endingMenuBtn->setBrush(in ? QBrush(QColor(50,80,150,230)) : QBrush(QColor(30,50,90,210)));
        }
    }

    // ====== 处理待执行的状态转换动作（从eventFilter鼠标点击触发） ======
    if (pendingAction != ACT_NONE) {
        PendingAction act = pendingAction;
        pendingAction = ACT_NONE; // 先清除，防止重入

        switch (act) {
        case ACT_SHOW_LEVEL_SELECT:
            cleanupMainMenuUI();
            showLevelSelect();
            break;
        case ACT_CONTINUE_GAME:
            if (currentState == LevelSelect) {
                cleanupSelectUI();
            } else {
                cleanupMainMenuUI();
            }
            currentState = PLAYING;
            loadLevel(savedLevelNum);
            player->setPos(savedCheckpointPos.x(), savedCheckpointPos.y());
            player->hp = savedHP;
            player->stamina = savedStamina;
            player->currentForm = savedForm;
            player->attackPowerTimer = savedAttackPowerTimer;
            player->invulnTimer = 90;
            lastCheckpointPos = savedCheckpointPos;
            savedLevelNum = 0;
            break;
        case ACT_SHOW_SETTINGS:
            cleanupMainMenuUI();
            showSettings();
            break;
        case ACT_SHOW_MAIN_MENU:
            if (currentState == SETTINGS)
                cleanupSettingsUI();
            else if (currentState == PAUSED) {
                cleanupPauseUI();
                cleanupGameObjects();
            } else if (currentState == GAME_OVER) {
                cleanupGameOverUI();
                cleanupGameObjects();
            }
            showMainMenu();
            break;
        case ACT_RESUME_GAME:
            cleanupPauseUI();
            currentState = PLAYING;
            timer->start(16);
            break;
        case ACT_RESTART_CHECKPOINT:
            cleanupGameOverUI();

            // Boss战重开：清除现有Boss及弹幕，让入场动画重新播放
            if (pendingBossType > 0) {
                // 先恢复猪鲨的水转换，防止Tile指针悬空并还原地形
                if (dukeFishron) {
                    for (auto& rec : dukeFishron->activeWaterChanges) {
                        rec.tile->changeType(rec.originalType, rec.originalPixmap);
                        waters.removeOne(rec.tile);
                        floors.append(rec.tile);
                    }
                    dukeFishron->activeWaterChanges.clear();
                    dukeFishron->pendingWaterConversions.clear();
                }
                // 从场景和敌人列表中移除所有Boss及相关单位
                for (int i = enemies.size() - 1; i >= 0; i--) {
                    Enemy* e = enemies[i];
                    if (e == dukeFishron || e == brainOfCthulhu || e == iceGod ||
                        dynamic_cast<Xuehua*>(e)) {
                        scene->removeItem(e);
                        enemies.removeAt(i);
                        delete e;
                    }
                }
                dukeFishron = nullptr;
                brainOfCthulhu = nullptr;
                iceGod = nullptr;
                bossSpawned = false;      // 允许入场动画重新触发
                bossIntroActive = false;
                bossIntroTimer = 0;
                // 清理残留弹幕
                for (auto* p : projectiles) { scene->removeItem(p); delete p; }
                projectiles.clear();
                // 重置猪鲨二阶段背景
                for (QGraphicsRectItem* bg : phase2BgLayers) {
                    bg->setOpacity(0.0);
                }
                for (QGraphicsRectItem* bg : backgroundLayers) {
                    bg->setOpacity(1.0);
                }
                dukePhase2BgStarted = false;
                phase2BgFadeTimer = 0;
            }

            player->setPos(lastCheckpointPos.x(), lastCheckpointPos.y());
            player->hp = 3;
            player->invulnTimer = 90;
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
            currentState = PLAYING;
            timer->start(16);
            break;
        case ACT_EXIT:
            this->close();
            break;
        default:
            break;
        }
        return; // 动作执行后跳过本帧其余逻辑
    }

    if (currentState == LevelSelect) {
        if (modeSelection == 1) {
            modeSelection = 0;
            currentState = PLAYING;
            cleanupSelectUI();
            loadLevel(1);                // 冒险模式 = 第一关
        }
        else if (modeSelection >= 21 && modeSelection <= 23) {
            pendingBossType = modeSelection - 20;  // 先读！21→1(克苏鲁), 22→2(猪鲨), 23→3(冰雪)
            modeSelection = 0;                      // 再清0
            currentState = PLAYING;
            cleanupSelectUI();
            loadLevel(99);
        }
        return; // 拦截
    }

    // ====== 结束动画状态 ======
    	if (currentState == ENDING) {
        endingAnimTimer++;

        // 锁死相机到固定位置
        view->centerOn(endingBaseX, endingBaseY);

        // 用固定相机中心直接计算视口四角，确保始终居中不跟随角色
        qreal halfViewW = view->viewport()->width() / 2.0;
        qreal halfViewH = view->viewport()->height() / 2.0;
        qreal cx = endingBaseX - halfViewW;
        qreal cy = endingBaseY - halfViewH;
        qreal vw = view->viewport()->width();
        qreal vh = view->viewport()->height();

        // 阶段0 (4秒): 卡比挥手 + 鸟儿从右向左飞
        if (endingStage == 0) {
            // 卡比挥手动画
            if (endingKirby && !endingKirbyFrames.isEmpty()) {
                int frame = (endingAnimTimer / 8) % endingKirbyFrames.size();
                QPixmap scaledFrame = endingKirbyFrames[frame].scaled(130, 130, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                endingKirby->setPixmap(scaledFrame);
            }

            // 大鸟从右向左直线飞行
            if (endingBigBird && !endingBigBirdFrames.isEmpty()) {
                int bf = (endingAnimTimer / 10) % endingBigBirdFrames.size();
                QPixmap scaledBird = endingBigBirdFrames[bf].scaled(70, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                endingBigBird->setPixmap(scaledBird);

                qreal birdX = cx + vw + 50 - (endingAnimTimer * 4.5);
                if (birdX < cx - 100) birdX = cx + vw + 50;
                qreal birdY = cy + vh * 0.42 - 35 + std::sin(endingAnimTimer * 0.05) * 8;
                endingBigBird->setPos(birdX, birdY);
            }

            // 小鸟从右向左直线飞行
            if (endingSmallBird && !endingSmallBirdFrames.isEmpty()) {
                int sf = (endingAnimTimer / 12) % endingSmallBirdFrames.size();
                QPixmap scaledBird = endingSmallBirdFrames[sf].scaled(45, 45, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                endingSmallBird->setPixmap(scaledBird);

                qreal sBirdX = cx + vw + 100 - (endingAnimTimer * 5.5);
                if (sBirdX < cx - 100) sBirdX = cx + vw + 100;
                qreal sBirdY = cy + vh * 0.42 + 5 + std::cos(endingAnimTimer * 0.04) * 6;
                endingSmallBird->setPos(sBirdX, sBirdY);
            }

            if (endingAnimTimer >= 240) {
                endingAnimTimer = 0;
                endingStage = 1;
            }
        }
        // 阶段1: ending.png 淡入
        else if (endingStage == 1) {
            if (!endingScreen) {
                QPixmap endingPix(":/tu/jieshudonghua/ending.png");
                if (!endingPix.isNull()) {
                    endingScreen = new QGraphicsPixmapItem(
                        endingPix.scaled(vw, vh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                    endingScreen->setPos(cx, cy);
                    endingScreen->setZValue(3010);
                    endingScreen->setOpacity(0.0);
                    scene->addItem(endingScreen);
                }
            }

            // 淡入效果 (前30帧从透明到不透明)
            if (endingScreen) {
                qreal fadeProgress = qMin(endingAnimTimer / 30.0, 1.0);
                endingScreen->setOpacity(fadeProgress);
            }

            // 鸟儿继续飞
            if (endingBigBird && !endingBigBirdFrames.isEmpty()) {
                int bf = (endingAnimTimer / 10) % endingBigBirdFrames.size();
                endingBigBird->setPixmap(endingBigBirdFrames[bf].scaled(70, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                qreal birdX = cx + vw + 50 - (endingAnimTimer * 4.5);
                if (birdX < cx - 100) birdX = cx + vw + 50;
                qreal birdY = cy + vh * 0.42 - 35 + std::sin(endingAnimTimer * 0.05) * 8;
                endingBigBird->setPos(birdX, birdY);
            }
            if (endingSmallBird && !endingSmallBirdFrames.isEmpty()) {
                int sf = (endingAnimTimer / 12) % endingSmallBirdFrames.size();
                endingSmallBird->setPixmap(endingSmallBirdFrames[sf].scaled(45, 45, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                qreal sBirdX = cx + vw + 100 - (endingAnimTimer * 5.5);
                if (sBirdX < cx - 100) sBirdX = cx + vw + 100;
                qreal sBirdY = cy + vh * 0.42 + 5 + std::cos(endingAnimTimer * 0.04) * 6;
                endingSmallBird->setPos(sBirdX, sBirdY);
            }

            if (endingAnimTimer >= 120) {
                endingAnimTimer = 0;
                endingStage = 2;
            }
        }
        // 阶段2: 显示"回到主菜单"按钮
        else if (endingStage == 2) {
            if (!endingMenuBtn) {
                qreal btnW = 220, btnH = 50;
                qreal btnX = cx + (SCREEN_W - btnW) / 2.0;
                qreal btnY = cy + vh - 100;

                endingMenuBtn = new QGraphicsRectItem(btnX, btnY, btnW, btnH);
                endingMenuBtn->setBrush(QBrush(QColor(30, 50, 90, 210)));
                endingMenuBtn->setPen(QPen(QColor(80, 120, 220), 2));
                endingMenuBtn->setZValue(3011);
                scene->addItem(endingMenuBtn);

                endingMenuBtnText = new QGraphicsTextItem("回到主菜单");
                endingMenuBtnText->setFont(QFont("SimHei", 18, QFont::Bold));
                endingMenuBtnText->setDefaultTextColor(Qt::white);
                endingMenuBtnText->setZValue(3012);
                qreal tw = endingMenuBtnText->boundingRect().width();
                endingMenuBtnText->setPos(btnX + (btnW - tw) / 2.0, btnY + 8);
                scene->addItem(endingMenuBtnText);
            }
        }
        return;
    }

    // ====== 非游玩状态下跳过游戏逻辑（防止玩家删除后空指针崩溃） ======
    if (currentState == MAIN_MENU || currentState == SETTINGS || currentState == PAUSED || currentState == GAME_OVER)
        return;

    // 阶段二：按下回车，卡比和地面一起左移
    /*if (currentState == PLAYING) {
        for (Enemy* e : enemies) e->setVisible(true);
        for (Cake* c : cakes) c->setVisible(true);
        for (Checkpoint* cp : checkpoints) cp->setVisible(true);
        player->isOnGround = true;
        qreal moveSpeed = 8.0;
        player->vx = 0; player->vy = 0;
        player->setPos(player->x() - moveSpeed, 856); // 贴地平移

        // 👇 核心修改 1：地板、怪物和道具一起跟着向左平移，保持相对地图的绝对位置
        for (Tile* tile : floors) {
            tile->setPos(tile->x() - moveSpeed, tile->y());
        }
        for (Tile* tile : waters) {
            tile->setPos(tile->x() - moveSpeed, tile->y());
        }
        for (Enemy* e : enemies) {
            e->setPos(e->x() - moveSpeed, e->y());
        }
        for (Cake* c : cakes) {
            c->setPos(c->x() - moveSpeed, c->y());
        }
        for (Star* s : stars) {
            s->setPos(s->x() - moveSpeed, s->y());
        }
        for (Checkpoint* cc : checkpoints){
            cc->setPos(cc->x() - moveSpeed, cc->y());
        }
        // 到达原定位置
        if (player->x() <= 100) {
            qreal errorX = 100 - player->x();
            // 👇 核心修改 2：最后一帧对齐误差校准时，怪物和道具也要同步校准
            for (Tile* tile : floors) {
                tile->setPos(tile->x() + errorX, tile->y());
            }
            for (Tile* tile : waters) {
                tile->setPos(tile->x() + errorX, tile->y());
            }
            for (Enemy* e : enemies) {
                e->setPos(e->x() + errorX, e->y());
            }
            for (Cake* c : cakes) {
                c->setPos(c->x() + errorX, c->y());
            }
            for (Star* s : stars) {
                s->setPos(s->x() + errorX, s->y());
            }
            player->setPos(100, 856); // 完美归位

            // 切换状态，唤醒所有元素
            currentState = PLAYING;
            staminaBar->setVisible(true);

        }
        staminaBar->setVisible(true);

        player->updateLogic();
        view->centerOn(500, 900);
        for (QGraphicsRectItem* bg : backgroundLayers) {
            bg->setPos(500 - bg->rect().width() / 2.0, 0);
        }
        return;
    }*/

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
    // ====== 1. 先检测是否在水中 ======
    player->inWater = false;
    Tile* currentSurface = nullptr;
\
    for (Tile* tile : waters) { // 如果你没有 waters 列表，可以遍历包含水的列表
        if (player->sceneBoundingRect().intersects(tile->sceneBoundingRect())) {
            player->inWater = true;
            if (tile->tileType() == Tile::WaterSurface) {
                currentSurface = tile; // 记录接触到的水面
            }
            break;
        }
    }

    // ====== 2. 水下物理与操作 ======
    if (player->inWater) {
        player->stamina-=0.5;
        // 1. 缓慢下沉 (替代原本的重力加速)
        player->vy += 0.2; // 比空气中重力加得慢
        if (player->vy > 2) player->vy = 2; // 水中最大下落速度被限制

        // 2. 按W键上浮
        if (keys.contains(Qt::Key_W)) {
            player->vy = -3.0; // 给予向上的速度
        }

        // 3. 水面悬浮判定 (半个身子在水外)
        if (currentSurface != nullptr) {
            double halfBodyY = player->y() + player->sceneBoundingRect().height() / 2.0;
            // 如果卡比的半腰高过了水面，且正在往上游
            if (halfBodyY < currentSurface->y() && player->vy < 0) {
                player->setY(currentSurface->y() - player->sceneBoundingRect().height() / 2.0);
                player->vy = 0; // 顶住水面，不再上升
            }
        }

    }
    else if (!player->isOnGround&& !player->isLightningFlying) {
        if (player->isHovering && player->vy > 0) {
            player->vy += 0.1;
            if (player->vy > 2.5) player->vy = 2.5;
        } else {
            player->vy += 0.8;
            if (player->vy > 15) player->vy = 15;
        }
    }
    if(!player->inWater){
        if (player->stamina < player->maxStamina) {
            player->stamina += 4;

            if (player->stamina > player->maxStamina) {
                player->stamina = player->maxStamina;
            }
        }
    }

    if (staminaBar) {
        staminaBar->setValue(player->stamina);

        // 【细节优化】：如果体力满了，可以隐藏体力条；不满时再显示，让画面更干净
        if (player->stamina >= player->maxStamina) {
            staminaBar->setVisible(false); // 满体力隐藏
        } else {
            staminaBar->setVisible(true);  // 消耗体力时显现
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

    player->inWater = false;
    currentSurface = nullptr;

    // 假设水方块存在一个 waters 列表里，或者你遍历所有场景物品
    for (Tile* tile : waters) { // 如果你没有 waters 列表，可以遍历包含水的列表
        if (player->sceneBoundingRect().intersects(tile->sceneBoundingRect())) {
            player->inWater = true;
            if (tile->tileType() == Tile::WaterSurface) {
                currentSurface = tile; // 记录接触到的水面
            }
            break;
        }
    }
    // 替换原有的 constFloors 遍历为倒序遍历
    {   QRectF pz = player->sceneBoundingRect().adjusted(-100,-100,100,50);
    for (int i = floors.size() - 1; i >= 0; i--) {
        Tile *tile = floors[i];
        if (!tile->sceneBoundingRect().intersects(pz)) continue;
        if (player->collidesWithItem(tile)) {

            // ====== 技能互动：火形态疾跑撞击特殊方块 ======
            if (player->isFireSprinting) {
                if (tile->tileType() == Tile::IceBlock) {
                    // 火融冰：改变地形贴图，由于是水了，不产生物理阻挡，直接 continue
                    tile->changeType(Tile::WaterBody, waterBodyPix);
                    waters.append(tile);
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

            /*if (player->inWater) {
                // 比较砖块的顶端 Y 坐标 和 玩家的底端 Y 坐标
                double tileTop = tile->sceneBoundingRect().top();

                // 如果砖块顶端和卡比脚底的高度差在一定范围内（比如一格以内）
                if (tileTop + renderSize >= player->y()) {
                    // 允许爬上去：强行把卡比拔高到砖块上
                    player->setY(tileTop - player->sceneBoundingRect().height());
                    continue; // 跳过撞墙阻挡，让玩家顺利前移
                }
            }*/
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
    }

    // 5. 垂直检测
    bool onGround = false;

    // A. 预判地面 (向下探测1像素)
    player->setPos(player->x(), player->y() + 1);
    {   QRectF pz = player->sceneBoundingRect().adjusted(-100,-100,100,50);
    for (int i = floors.size() - 1; i >= 0; i--) {
        Tile *tile = floors[i];
        if (!tile->sceneBoundingRect().intersects(pz)) continue;
        if (player->collidesWithItem(tile)) {

            // ====== 技能互动：火形态疾跑上下方向融冰与炸石 ======
            if (player->isFireSprinting) {
                if (tile->tileType() == Tile::IceBlock) {
                    // 火融冰：改变地形贴图并移除物理体积
                    tile->changeType(Tile::WaterBody, waterBodyPix);
                    waters.append(tile);
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
    }

    // B. 正式垂直移动与碰撞
    if (!onGround) {
        player->setPos(player->x(), player->y() - 1 + player->vy);
        { QRectF pz = player->sceneBoundingRect().adjusted(-100,-100,100,50);
        for (int i = floors.size() - 1; i >= 0; i--) {
            Tile *tile = floors[i];
            if (!tile->sceneBoundingRect().intersects(pz)) continue;
            if (player->collidesWithItem(tile)) {

                // ====== 技能互动：火形态疾跑上下方向融冰与炸石 ======
                if (player->isFireSprinting) {
                    if (tile->tileType() == Tile::IceBlock) {
                        tile->changeType(Tile::WaterBody, waterBodyPix);
                        waters.append(tile);
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
    }
    player->isOnGround = onGround;
    // ====== 体力消耗：使用中心点检测水中状态（避免边界误触） ======
    {
        bool finalInWater = false;
        QPointF playerCenter = player->sceneBoundingRect().center();
        for (Tile* tile : waters) {
            if (tile->sceneBoundingRect().contains(playerCenter)) {
                finalInWater = true;
                break;
            }
        }
        if (finalInWater) {
            if (keys.contains(Qt::Key_W) || keys.contains(Qt::Key_A) || keys.contains(Qt::Key_D)) {
                player->stamina -= 1.5;
                if (player->stamina <= 0) {
                    player->hp = 0;
                }
            }
        }
    }
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

    // ====== 浮动文字触发与更新 ======
    for (auto& t : tutorialTriggers) {
        if (!t.shown && player && player->x() >= t.triggerX)
            triggerTutorialText(&t);
    }
    updateTutorialTexts();

    // ====== 脚步音效 ======
    {
        bool shouldStep = (player->currentState == Player::WALKING || player->currentState == Player::FATTY_WALKING)
                          && player->isOnGround;
        if (shouldStep && stepPlayer->playbackState() != QMediaPlayer::PlayingState)
            stepPlayer->play();
        else if (!shouldStep && stepPlayer->playbackState() == QMediaPlayer::PlayingState)
            stepPlayer->stop();
    }

    // ====== 水声循环 ======
    {
        if (player->inWater && waterPlayer->playbackState() != QMediaPlayer::PlayingState)
            waterPlayer->play();
        else if (!player->inWater && waterPlayer->playbackState() == QMediaPlayer::PlayingState)
            waterPlayer->stop();
    }

    // ====== 火形态爆炸音效（边缘检测，仅爆炸开始时播一次） ======
    if (player->isExploding && !prevExploding) {
        sfxPlayer->setSource(QUrl("qrc:///tu/music/JDSherbert - Pixel Explosions SFX Pack - EMP (1).mp3"));
        sfxAudio->setVolume(0.5);
        sfxPlayer->play();
    }
    prevExploding = player->isExploding;


    // ====== 运动方块更新（会动的刺等） ======
    for (Tile* tile : floors) {
        tile->updateLogic();
    }

    // ====== 敌人物理与逻辑计算 ======
    for (Enemy* enemy : enemies) {
        if (enemy->isDead) continue;

        // 1. 更新敌人自身逻辑 (决定 vx 和动画)
        enemy->updateLogic();

        if (!enemy->ignoresTiles) {
            bool alreadyReversed = false;

            // 2. 先应用重力 (自由落体算法)
            enemy->vy += 0.8;
            if (enemy->vy > 15) enemy->vy = 15;

            // 3. 垂直移动与地面碰撞（先垂直后水平，防止地面误判为墙壁导致抖动）
            enemy->setPos(enemy->x(), enemy->y() + enemy->vy);
            QRectF eRect = enemy->sceneBoundingRect();
            { QRectF ez = eRect.adjusted(-100,-100,100,50);
            for (Tile *tile : floors) {
                if (!tile->sceneBoundingRect().intersects(ez)) continue;
                if (enemy->collidesWithItem(tile)) {
                    QRectF tRect = tile->sceneBoundingRect();
                    if (enemy->vy > 0) { // 往下掉时踩到地板
                        enemy->setPos(enemy->x(), tRect.top() - eRect.height());
                        enemy->vy = 0; // 落地速度清零
                    }
                    break;
                }
            }
            }

            // 4. 水平移动与墙壁碰撞（reversesOnCollision=false时跳过，防止Boss小怪卡墙抖动）
            enemy->setPos(enemy->x() + enemy->vx, enemy->y());
            if (enemy->reversesOnCollision) {
                eRect = enemy->sceneBoundingRect();
                { QRectF ez = eRect.adjusted(-100,-100,100,50);
                for (Tile *tile : floors) {
                    if (!tile->sceneBoundingRect().intersects(ez)) continue;
                    QRectF tRect = tile->sceneBoundingRect();
                    if (enemy->collidesWithItem(tile)) {
                        // 物理阻挡
                        if (enemy->vx > 0) {
                            enemy->setPos(tRect.left() - eRect.width(), enemy->y());
                        } else if (enemy->vx < 0) {
                            enemy->setPos(tRect.right(), enemy->y());
                        }
                        // 撞墙后掉头（用本地标记防止同帧内重复掉头）
                        if (!alreadyReversed) {
                            enemy->reverseDirection();
                            alreadyReversed = true;
                        }
                        break;
                    }
                }
                }
                qreal lookAheadX = (enemy->vx > 0) ? eRect.right() + 7 : eRect.left() - 7;
                qreal footY = eRect.bottom() + 5;
                QPointF checkPoint(lookAheadX, footY);

                bool hasFloorAhead = false;
                { QRectF cz(checkPoint.x() - 1, checkPoint.y() - 1, 2, 2);
                for (Tile *tile : floors) {
                    if (!tile->sceneBoundingRect().intersects(cz)) continue;
                    if (tile->sceneBoundingRect().contains(checkPoint)) {
                        hasFloorAhead = true;
                        break;
                    }
                }
                }

                // 如果前方没有地面，到边缘掉头
                if (!hasFloorAhead && !alreadyReversed) {
                    enemy->reverseDirection();
                    alreadyReversed = true;
                }
            }
        } else {
            // Boss无视地形，直接应用速度
            enemy->setPos(enemy->x() + enemy->vx, enemy->y());
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
    {
        qreal rightBound = sceneW;
        // Boss战地图限制相机范围
        if (currentLevelNum == 99) {
            qreal levelW = 80 * renderSize;
            rightBound = levelW;
        }
        if (cameraX < halfViewW) {
            cameraX = halfViewW;
        } else if (cameraX > rightBound - halfViewW) {
            cameraX = rightBound - halfViewW;
        }
    }

    // 3. 执行视角居中并加入【震屏特效】
    qreal renderX = cameraX;
    qreal renderY = cameraY;

    if (player->isExploding) {
        // 使用随机数让相机在中心点周围 ±8 像素剧烈抖动
        renderX += (rand() % 17) - 8;
        renderY += (rand() % 17) - 8;
    }

    // Boss登场屏幕震荡（必须在 centerOn 之前修改 renderX/renderY）
    if (bossIntroActive && bossIntroTimer >= 120) {
        int intensity = qMin(12, 4 + bossIntroTimer / 10);
        renderX += (rand() % (intensity*2+1)) - intensity;
        renderY += (rand() % (intensity*2+1)) - intensity;
    }

    view->centerOn(renderX, renderY);

    // 4. 背景图层跟随”锁定后且未震动”的相机坐标
    // 这样背景不会跟着画面一起疯狂乱抖，产生极好的纵深对比感
    for (QGraphicsRectItem* bg : backgroundLayers) {
        qreal bgX = cameraX - bg->rect().width() / 2.0;
        bg->setPos(bgX, 0);
    }
    for (QGraphicsRectItem* bg : phase2BgLayers) {
        qreal bgX = cameraX - bg->rect().width() / 2.0;
        bg->setPos(bgX, 0);
    }

    // ====== Boss动态生成：开始登场动画 ======
    if (pendingBossType > 0 && !bossSpawned && !bossIntroActive && player && player->x() > bossSpawnX - 300) {
        bossIntroActive = true;
        bossIntroTimer = 0;
    }

    // ====== Boss登场动画流程 ======
    if (bossIntroActive) {
        bossIntroTimer++;

        // 阶段1：前2秒安静等待
        if (bossIntroTimer == 120) {
            // 停止BGM
            bgmPlayer->stop();

            // 播放 horror stinger
            bossStingerPlayer->setSource(QUrl("qrc:///tu/music/horror_01_stinger_impact_glitch_01.ogg"));
            bossStingerAudio->setVolume(volumeLevel);
            bossStingerPlayer->play();

            // 红色遮罩（大幅超出屏幕范围，防止边缘露出）
            bossIntroOverlay = new QGraphicsRectItem(0, 0, SCREEN_W + 1000, SCREEN_H + 1000);
            bossIntroOverlay->setBrush(QBrush(QColor(180, 0, 0, 120)));
            bossIntroOverlay->setPen(Qt::NoPen);
            bossIntroOverlay->setZValue(3000);
            scene->addItem(bossIntroOverlay);

            // 巨大白色感叹号
            bossIntroExclamation = new QGraphicsTextItem("!");
            bossIntroExclamation->setFont(QFont("Arial Black", 140, QFont::Bold));
            bossIntroExclamation->setDefaultTextColor(QColor(255, 255, 255));
            bossIntroExclamation->setZValue(3001);
            double ew = bossIntroExclamation->boundingRect().width();
            bossIntroExclamation->setPos(cameraX - ew/2.0, cameraY - 70);
            scene->addItem(bossIntroExclamation);
        }

        // 每帧更新遮罩和感叹号位置（跟随镜头）
        if (bossIntroOverlay) {
            bossIntroOverlay->setPos(cameraX - halfViewW - 500, cameraY - SCREEN_H/2.0 - 500);
        }
        if (bossIntroExclamation) {
            double ew = bossIntroExclamation->boundingRect().width();
            bossIntroExclamation->setPos(cameraX - ew/2.0, cameraY - 70);
        }

        // 阶段2：3秒后（总5秒）生成Boss，切换音乐
        if (bossIntroTimer == 300) {
            // 生成Boss
            if (pendingBossType == 1) {
                brainOfCthulhu = new BrainOfCthulhu(player);
                brainOfCthulhu->setPos(bossSpawnX, bossSpawnY);
                brainOfCthulhu->setVisible(true);
                scene->addItem(brainOfCthulhu);
                enemies.append(brainOfCthulhu);
            } else if (pendingBossType == 2) {
                dukeFishron = new DukeFishron(player);
                dukeFishron->setPos(bossSpawnX, bossSpawnY);
                dukeFishron->setVisible(true);
                scene->addItem(dukeFishron);
                enemies.append(dukeFishron);
            } else if (pendingBossType == 3) {
                iceGod = new IceGod(player);
                iceGod->setPos(bossSpawnX, bossSpawnY);
                iceGod->setVisible(true);
                scene->addItem(iceGod);
                enemies.append(iceGod);
            }
            bossSpawned = true;

            // Boss登场时立刻在屏幕边缘生成一只元素小怪
            {
                Enemy::CopyAbility abilities[] = {Enemy::FIRE, Enemy::ICE, Enemy::LEAF, Enemy::SPARK};
                int idx = rand() % 4;
                Enemy::CopyAbility abil = abilities[idx];
                QString path;
                int frames = 6;
                double spd = 1.5, sc = 0.6;
                switch (abil) {
                case Enemy::FIRE:  path=":/tu/fire_enemy.png"; frames=5; spd=1.5; sc=2.0; break;
                case Enemy::ICE:   path=":/tu/Ice_Dude.png"; frames=6; spd=1.2; sc=0.6; break;
                case Enemy::LEAF:  path=":/tu/Leaf_Dude.png"; frames=8; spd=1.0; sc=0.6; break;
                case Enemy::SPARK: path=":/tu/Lightning_Dude.png"; frames=6; spd=1.8; sc=0.6; break;
                default: path=":/tu/Ice_Dude.png"; break;
                }
                bool left = rand() % 2 == 0;
                qreal sx = left ? (cameraX - halfViewW + 24) : (cameraX + halfViewW - 24);
                MinionEnemy* m = new MinionEnemy(path, frames, spd, abil);
                m->setScale(sc);
                m->setPatrolDuration(999999); // 禁用巡逻转向，仅靠悬崖检测
                m->setPos(sx, 0);
                scene->addItem(m);
                enemies.append(m);
                qreal eh = m->sceneBoundingRect().height();
                qreal gt = 1200;
                for (Tile* t : floors) {
                    QRectF tr = t->sceneBoundingRect();
                    if (sx >= tr.left() && sx <= tr.right() && tr.top() < gt) gt = tr.top();
                }
                m->setPos(sx, gt - eh);
                // 朝向屏幕内侧移动（左边缘→朝右，右边缘→朝左）避免悬崖检测抖动
                if (left && !m->isFacingRight()) m->reverseDirection();
                else if (!left && m->isFacingRight()) m->reverseDirection();
            }

            // 停止 horror stinger
            bossStingerPlayer->stop();

            // 随机播放一首RPG战斗音乐并循环
            int idx = rand() % bossMusicTracks.size();
            bgmPlayer->setSource(QUrl(bossMusicTracks[idx]));
            bgmPlayer->setLoops(QMediaPlayer::Infinite);
            bgmPlayer->play();

            // 清理登场特效
            if (bossIntroOverlay) { scene->removeItem(bossIntroOverlay); delete bossIntroOverlay; bossIntroOverlay = nullptr; }
            if (bossIntroExclamation) { scene->removeItem(bossIntroExclamation); delete bossIntroExclamation; bossIntroExclamation = nullptr; }
            bossIntroActive = false;
            bossIntroTimer = 0;
        }
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
        double barY = cameraY - view->viewport()->height() / 2.0 + 20;
        bossHpBarBg->setVisible(true);
        bossHpBarBg->setRect(0, 0, barW, barH);
        bossHpBarBg->setPos(barX, barY);
        bossHpBarFg->setVisible(true);
        double ratio = (double)activeBoss->hp / bossFullHp;
        if (ratio < 0) ratio = 0;
        bossHpBarFg->setRect(0, 0, barW * ratio, barH);
        bossHpBarFg->setPos(barX, barY);
        // 猪鲨根据阶段改变血条颜色
        if (activeBoss == dukeFishron) {
            if (dukeFishron->isPhase3)
                bossHpBarFg->setBrush(QBrush(QColor(255, 100, 50)));  // 橙色
            else if (dukeFishron->isPhase2)
                bossHpBarFg->setBrush(QBrush(QColor(100, 150, 255))); // 蓝色
            else
                bossHpBarFg->setBrush(QBrush(QColor(220, 30, 30)));   // 红色（一阶段）
        } else {
            bossHpBarFg->setBrush(QBrush(QColor(220, 30, 30)));       // 红色
        }
    } else {
        if (bossHpBarBg) bossHpBarBg->setVisible(false);
        if (bossHpBarFg) bossHpBarFg->setVisible(false);
    }

    // ====== Boss战每30秒在屏幕左右随机生成属性小怪 ======
    if (activeBoss && activeBoss->hp > 0) {
        bossMinionSpawnTimer++;
        if (bossMinionSpawnTimer >= 1800) { // 30秒 = 1800帧 (60fps)
            bossMinionSpawnTimer = 0;

            // 随机属性
            Enemy::CopyAbility abilities[] = {Enemy::FIRE, Enemy::ICE, Enemy::LEAF, Enemy::SPARK};
            int idx = rand() % 4;
            Enemy::CopyAbility ability = abilities[idx];

            // 根据属性选贴图/帧数/速度/缩放
            QString spritePath;
            int frames = 6;
            double speed = 1.5;
            double scale = 0.6;
            switch (ability) {
            case Enemy::FIRE:
                spritePath = ":/tu/fire_enemy.png"; frames = 5; speed = 1.5; scale = 2.0; break;
            case Enemy::ICE:
                spritePath = ":/tu/Ice_Dude.png";  frames = 6; speed = 1.2; scale = 0.6; break;
            case Enemy::LEAF:
                spritePath = ":/tu/Leaf_Dude.png"; frames = 8; speed = 1.0; scale = 0.6; break;
            case Enemy::SPARK:
                spritePath = ":/tu/Lightning_Dude.png"; frames = 6; speed = 1.8; scale = 0.6; break;
            default:
                spritePath = ":/tu/Ice_Dude.png"; break;
            }

            // 随机左右侧，在屏幕边缘生成（在屏幕内，确保地面存在）
            bool spawnLeft = rand() % 2 == 0;
            qreal spawnX = spawnLeft ? (cameraX - halfViewW + 24) : (cameraX + halfViewW - 24);

            // 先创建小怪并加到场景，获取其碰撞高度
            MinionEnemy* minion = new MinionEnemy(spritePath, frames, speed, ability);
            minion->setScale(scale);
            minion->setPatrolDuration(999999); // 禁用巡逻转向
            minion->reversesOnCollision = false; // 取消撞墙/悬崖回头，防止抖动
            minion->setPos(spawnX, 0);
            scene->addItem(minion);
            enemies.append(minion);

            // 扫描地面，找到脚下的地板顶部Y，直接站在上面（避免重力落地和悬崖检测抖动）
            qreal enemyH = minion->sceneBoundingRect().height();
            qreal groundTop = 1200; // 兜底
            for (Tile* tile : floors) {
                QRectF tr = tile->sceneBoundingRect();
                if (spawnX >= tr.left() && spawnX <= tr.right()) {
                    if (tr.top() < groundTop) {
                        groundTop = tr.top();
                    }
                }
            }
            minion->setPos(spawnX, groundTop - enemyH);
            // 朝向屏幕内侧移动（左边缘→朝右，右边缘→朝左）避免悬崖检测抖动
            if (spawnLeft && !minion->isFacingRight()) minion->reverseDirection();
            else if (!spawnLeft && minion->isFacingRight()) minion->reverseDirection();
        }
    } else {
        bossMinionSpawnTimer = 0;
    }

    // ====== Boss战星星生成（每5-10秒生成一颗） ======
    if (activeBoss && activeBoss->hp > 0) {
        bossStarSpawnTimer++;
        int interval = 300 + (rand() % 300); // 5~10秒随机间隔
        if (bossStarSpawnTimer >= interval) {
            bossStarSpawnTimer = 0;

            // 在屏幕可见范围内随机X位置生成星星
            qreal sx = cameraX - halfViewW + 50 + (rand() % (int)(SCREEN_W - 100));
            Star* star = new Star();
            star->setPos(sx, cameraY - halfViewW); // 屏幕顶部上方生成
            scene->addItem(star);
            stars.append(star);
        }
    } else {
        bossStarSpawnTimer = 0;
    }

    // ====== DukeFishron二阶段背景过渡（60帧交叉淡入淡出） ======
    if (dukeFishron && dukeFishron->phase2BgTriggered && !dukePhase2BgStarted) {
        dukePhase2BgStarted = true;
        phase2BgFadeTimer = 0;
    }
    if (dukePhase2BgStarted && phase2BgFadeTimer < 60) {
        phase2BgFadeTimer++;
        double t = phase2BgFadeTimer / 60.0;
        for (QGraphicsRectItem* bg : backgroundLayers) {
            bg->setOpacity(1.0 - t);
        }
        for (QGraphicsRectItem* bg : phase2BgLayers) {
            bg->setOpacity(t);
        }
    }

    // ====== 形态技能冷却图标 (左下角) ======
    // 先隐藏所有图标
    if (fireCdIcon) fireCdIcon->setVisible(false);
    if (iceCdIcon)  iceCdIcon->setVisible(false);
    if (leafCdIcon) leafCdIcon->setVisible(false);
    if (cooldownText) cooldownText->setVisible(false);

    // 根据当前形态显示对应的冷却图标
    auto showCdIcon = [&](QGraphicsPixmapItem* icon, int cdTimer, const QString &label) {
        // 后备：如果图标无效或图片为空，用文字显示
        if (!icon || icon->pixmap().isNull()) {
            if (cooldownText) {
                QString text = (cdTimer > 0)
                    ? QString("%1: %2s").arg(label).arg((cdTimer + 59) / 60)
                    : label + ": 就绪";
                cooldownText->setPlainText(text);
                qreal scrRight  = cameraX + halfViewW;
                qreal scrBottom = cameraY + view->viewport()->height() / 2.0;
                qreal tw = cooldownText->boundingRect().width();
                qreal th = cooldownText->boundingRect().height();
                cooldownText->setPos(scrRight - tw - 20, scrBottom - th - 20);
                cooldownText->setDefaultTextColor((cdTimer > 0) ? Qt::gray : QColor(255, 255, 200));
                cooldownText->setVisible(true);
            }
            return;
        }
        // 固定在左下角
        qreal screenLeft = cameraX - halfViewW;
        qreal screenBottom = cameraY + view->viewport()->height() / 2.0;
        icon->setPos(screenLeft + 20, screenBottom - 60);
        icon->setVisible(true);
        // 冷却中变暗淡，可用时高亮
        icon->setOpacity((cdTimer > 0) ? 0.35 : 1.0);
    };

    switch (player->currentForm) {
        case Enemy::FIRE: showCdIcon(fireCdIcon, player->fireSkillCooldownTimer, "火疾跑"); break;
        case Enemy::ICE:  showCdIcon(iceCdIcon,  player->iceDefendCooldownTimer, "冰防御"); break;
        case Enemy::LEAF: showCdIcon(leafCdIcon, player->leafSkillCooldownTimer, "叶羽毛"); break;
        default: break;
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

    // ====== 消费猪鲨待发射弹幕 ======
    if (dukeFishron && !dukeFishron->isDead && dukeFishron->isPhase2) {
        for (Projectile* p : dukeFishron->pendingProjectiles) {
            scene->addItem(p);
            projectiles.append(p);
        }
        dukeFishron->pendingProjectiles.clear();
    }

    // ====== 猪鲨二阶段：处理水转换 ======
    if (dukeFishron && !dukeFishron->isDead && dukeFishron->isPhase2) {
        // 1. 处理待转换水块
        for (Tile* tile : dukeFishron->pendingWaterConversions) {
            int idx = floors.indexOf(tile);
            if (idx >= 0) {
                Tile::TileType origType = tile->tileType();
                QPixmap origPix = tile->pixmap();
                tile->changeType(Tile::WaterBody, waterBodyPix);
                waters.append(tile);
                floors.removeAt(idx);
                DukeFishron::WaterChangeRecord rec;
                rec.tile = tile;
                rec.originalType = origType;
                rec.originalPixmap = origPix;
                rec.timer = 0;
                dukeFishron->activeWaterChanges.append(rec);
            }
        }
        dukeFishron->pendingWaterConversions.clear();

        // 2. 处理活跃水块计时器（600帧=10秒后恢复）
        for (int i = dukeFishron->activeWaterChanges.size() - 1; i >= 0; i--) {
            auto& rec = dukeFishron->activeWaterChanges[i];
            rec.timer++;
            if (rec.timer >= 600) {
                rec.tile->changeType(rec.originalType, rec.originalPixmap);
                waters.removeOne(rec.tile);
                floors.append(rec.tile);
                dukeFishron->activeWaterChanges.removeAt(i);
            }
        }
    }

    // ====== 猪鲨死亡或进入三阶段：清理所有残留水块 ======
    if (dukeFishron && (dukeFishron->isDead || dukeFishron->isPhase3) && !dukeFishron->activeWaterChanges.isEmpty()) {
        for (auto& rec : dukeFishron->activeWaterChanges) {
            rec.tile->changeType(rec.originalType, rec.originalPixmap);
            waters.removeOne(rec.tile);
            floors.append(rec.tile);
        }
        dukeFishron->activeWaterChanges.clear();
        dukeFishron->pendingWaterConversions.clear();
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


        // 2. 新增：检测是否撞到实体方块（Boss弹幕无视地形）
        if (!hitEnemy && !proj->ignoresWalls) { // 如果已经打中敌人了，就不需要再检测撞墙了
            QRectF projZ = proj->sceneBoundingRect().adjusted(-50,-50,50,50);
            for (Tile *tile : floors) {
                if (!tile->sceneBoundingRect().intersects(projZ)) continue;
                // 只要子弹碰到了 floors 列表里的方块（实体墙、草地、石头等）
                if (proj->collidesWithItem(tile)) {
                    hitWall = true;
                    break;
                }
            }
            if (!hitWall) {
                for (Tile *tile : waters) {
                    if (!tile->sceneBoundingRect().intersects(projZ)) continue;
                    // 只要子弹碰到了 floors 列表里的方块（实体墙、草地、石头等）
                    if (proj->collidesWithItem(tile)) {
                        hitWall = true;
                        break;
                    }
                }
            }
        }
        // 2.5 检测Boss弹幕是否打到玩家
        bool hitPlayer = false;
        if (proj->hurtsPlayer && player->invulnTimer == 0 && player->hp > 0) {
            if (proj->collidesWithItem(player)) {
                hitPlayer = true;
                if (!player->isRolling && !player->isIceDefending && !player->isExploding) {
                    player->hp--;
                    // 扣血音效
                    sfxPlayer->setSource(QUrl("qrc:///tu/music/sitar_negative_quick.wav"));
                    sfxAudio->setVolume(0.5);
                    sfxPlayer->play();
                    player->invulnTimer = 90;
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
    // ====== 玩家与刺的碰撞检测（使用三角形内接最大矩形作为碰撞盒） ======
    if (player->invulnTimer == 0 && player->hp > 0) {

        for (Tile* spike : spikes) {

            // 计算尖刺三角形内接最大长方形碰撞盒
            QRectF sRect = spike->sceneBoundingRect();
            bool isWallSpike = (sRect.height() > sRect.width() * 2.5);
            QRectF spikeHitbox = isWallSpike
                ? QRectF(sRect.left() + sRect.width() / 2.0,
                         sRect.top() + sRect.height() / 4.0,
                         sRect.width() / 2.0,
                         sRect.height() / 2.0)
                : QRectF(sRect.left() + sRect.width() / 4.0,
                         sRect.top() + sRect.height() / 2.0,
                         sRect.width() / 2.0,
                         sRect.height() / 2.0);

            if (player->sceneBoundingRect().intersects(spikeHitbox)) {

                // 技能免伤保护
                if (player->isRolling || player->isIceDefending || player->isExploding) {
                    continue;
                }

                // 触发扣血
                player->hp--;
                // 扣血音效
                sfxPlayer->setSource(QUrl("qrc:///tu/music/sitar_negative_quick.wav"));
                sfxAudio->setVolume(0.5);
                sfxPlayer->play();
                player->invulnTimer = 90;
                player->vy = -6; // 受击向上弹

                // 根据速度方向反向弹飞（防卡墙）
                {
                    double pushX = 8;
                    double newX = (player->vx > 0) ? player->x() - pushX : player->x() + pushX;
                    player->setPos(newX, player->y());
                    // 如果弹飞后卡进墙里，改为向上弹
                    bool stuck = false;
                    { QRectF pz2 = player->sceneBoundingRect().adjusted(-10,-10,10,10);
                    for (Tile* tile : floors) {
                        if (!tile->sceneBoundingRect().intersects(pz2)) continue;
                        if (player->collidesWithItem(tile)) {
                            stuck = true;
                            break;
                        }
                    }
                    }
                    if (stuck) {
                        player->setPos(player->x(), player->y() - pushX);
                    }
                }
                break;
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

                // 【机制平衡】如果卡比正在翻滚、冰防御或雷冲刺，视为无敌/免伤，不扣血
                if (player->isRolling) continue;
                if (player->isIceDefending) continue;
                if (player->isLightningDashing) continue;
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
                // 扣血音效
                sfxPlayer->setSource(QUrl("qrc:///tu/music/sitar_negative_quick.wav"));
                sfxAudio->setVolume(0.5);
                sfxPlayer->play();
                player->invulnTimer = 90;
                player->vy = -6;
                break; // 单帧内只承受一次伤害
            }
        }
        } // if (player->hp > 0) 被刺伤后跳过敌人检测
    }

    // ====== 掉入虚空直接死亡 ======
    if (player->y() > scene->sceneRect().height() + 100) {
        player->hp = 0;
    }

    // ====== 统一死亡检查（刺、敌人或虚空都可能导致死亡）======
    if (player->hp <= 0) {
        // 停止脚步和水声
        if (stepPlayer->playbackState() == QMediaPlayer::PlayingState) stepPlayer->stop();
        if (waterPlayer->playbackState() == QMediaPlayer::PlayingState) waterPlayer->stop();
        // 死亡音效（暂停BGM）
        bgmPlayer->pause();
        bgmPaused = true;
        cinematicPlayer->setSource(QUrl("qrc:///tu/music/grand_piano_negative_long.wav"));
        cinematicAudio->setVolume(0.5);
        cinematicPlayer->play();

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
            scene->removeItem(gameOverText);
            delete gameOverText;
            showGameOver();
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

    // ====== 终点碰撞检测 ======
    for (Goal* g : goals) {
        if (!g->isReached && player->collidesWithItem(g)) {
            g->isReached = true;
            startEndingAnimation();
            return;
        }
    }

    // ====== 新增：生命值图标动态固定在屏幕左上角 (HUD) ======
    qreal screenLeft = cameraX - halfViewW;
    qreal screenTop = cameraY - view->viewport()->height() / 2.0;

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

        // 2.5 吞噬判定矩形（比身体更大，怪物进入此区域即被吞入，不需要贴脸）
        QRectF swallowEatRect;
        if (player->facingRight) {
            swallowEatRect = QRectF(player->x(), player->y(), 48 + 48, 48);
        } else {
            swallowEatRect = QRectF(player->x() - 48, player->y(), 48 + 48, 48);
        }

        // 3. 逆序检查场景中的怪物（拉扯与进肚）
        for (int j = enemies.size() - 1; j >= 0; j--) {
            Enemy* enemy = enemies[j];
            if (!enemy->isDead && enemy->canBeSwallowed()) {
                QRectF enemyRect = enemy->sceneBoundingRect();

                // 【怪物阶段一】：进入吞噬判定矩形 -> 真正吸进肚子里
                if (swallowEatRect.intersects(enemyRect)) {
                    player->swallowedAbility = enemy->ability;
                    enemy->isDead = true;
                    scene->removeItem(enemy);
                    enemies.removeAt(j);
                    delete enemy;

                    player->isFatty = true;
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

        // 5. 检查星星（吸入与吞噬）
        for (int j = stars.size() - 1; j >= 0; j--) {
            Star* star = stars[j];
            QRectF starRect = star->sceneBoundingRect();

            // 【星星阶段一】：被吸入到吞食判定范围 -> 吞噬获得单次攻击
            if (swallowEatRect.intersects(starRect)) {
                player->starAttackStock++;
                scene->removeItem(star);
                stars.removeAt(j);
                delete star;
                break;
            }
            // 【星星阶段二】：处于吸气范围内 -> 拉扯
            else if (swallowWindRect.intersects(starRect)) {
                double pullSpeed = 5.5;
                if (player->facingRight) {
                    star->setPos(star->x() - pullSpeed, star->y());
                } else {
                    star->setPos(star->x() + pullSpeed, star->y());
                }
            }
        }
    }
    // ====== 新增：监听卡比吐出动画的触发帧，生成对应的星星子弹 ======
    if (player->triggerSpitStar) {
        player->triggerSpitStar = false; // 消费信号，立即复位

        // 1. 创建高伤害、高速度的星星子弹
        Projectile* spitStar = new Projectile(player->facingRight);
        spitStar->damage = 50;
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
        { QRectF cakeZ = cRect.adjusted(-10,-10,10,10);
        for (Tile *tile : floors) {
            if (!tile->sceneBoundingRect().intersects(cakeZ)) continue;
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
    }
    // ====== 星星重力与地面碰撞 ======
    for (int i = stars.size() - 1; i >= 0; i--) {
        Star* star = stars[i];
        star->updateLogic();

        // 防止抖动：检测星星是否已停在地面上
        bool isResting = false;
        if (star->vy == 0) {
            QRectF sRectCheck = star->sceneBoundingRect();
            QPointF footPoint(sRectCheck.center().x(), sRectCheck.bottom() + 2);
            for (Tile* tile : floors) {
                if (tile->sceneBoundingRect().contains(footPoint)) {
                    isResting = true;
                    break;
                }
            }
        }

        // 1. 应用重力（仅当未停在地面时）
        if (!isResting) {
            star->vy += 0.8;
            if (star->vy > 15) star->vy = 15;
        }

        // 2. 垂直位移
        star->setPos(star->x(), star->y() + star->vy);

        // 3. 地板碰撞
        QRectF sRect = star->sceneBoundingRect();
        { QRectF starZ = sRect.adjusted(-10,-10,10,10);
        for (Tile* tile : floors) {
            if (!tile->sceneBoundingRect().intersects(starZ)) continue;
            if (star->collidesWithItem(tile)) {
                QRectF tRect = tile->sceneBoundingRect();
                if (star->vy > 0) {
                    star->setPos(star->x(), tRect.top() - sRect.height());
                    star->vy = 0;
                }
                break;
            }
        }
        }
    }
	// ====== 星星触碰获得（无需吞噬，碰到就自动收集） ======
	if (player && player->hp > 0) {
		QRectF pRect = player->sceneBoundingRect().adjusted(4, 4, -4, -4);
		for (int k = stars.size() - 1; k >= 0; k--) {
			if (pRect.intersects(stars[k]->sceneBoundingRect())) {
				player->starAttackStock++;
				Star* collected = stars[k];
				scene->removeItem(collected);
					stars.removeAt(k);
				delete collected;
				break;
			}
		}
	}
    // ====== 木箱物理（重力 + 地面碰撞） ======
    for (Crate* crate : crates) {
        crate->vy += 0.8;
        if (crate->vy > 15) crate->vy = 15;
        crate->setPos(crate->x(), crate->y() + crate->vy);

        QRectF crateRect = crate->sceneBoundingRect();
        { QRectF crateZ = crateRect.adjusted(-10,-10,10,10);
        for (Tile* tile : floors) {
            if (!tile->sceneBoundingRect().intersects(crateZ)) continue;
            if (crate->collidesWithItem(tile)) {
                QRectF tileRect = tile->sceneBoundingRect();
                if (crate->vy > 0) {
                    crate->setPos(crate->x(), tileRect.top() - crateRect.height());
                    crate->vy = 0;
                }
                break;
            }
        }
        }
    }

    // ====== 玩家推动木箱 ======
    if (player && player->hp > 0) {
        cratePushSoundPlayed = false;
        for (Crate* crate : crates) {
            QRectF pRect = player->sceneBoundingRect();
            QRectF cRect = crate->sceneBoundingRect();
            if (pRect.intersects(cRect)) {
                // 计算水平穿透方向，将玩家推出木箱
                // 根据玩家中心与木箱中心的相对位置判断方向
                bool playerOnRight = (player->x() > crate->x());
                double pushDir = playerOnRight ? -1 : 1;
                double pushSpeed = qMin(qAbs(cRect.right() - pRect.left()) + 1.0, 10.0); // 刚好推出

                // 先尝试推出玩家（防止卡进木箱内部）
                double sepX = playerOnRight ? cRect.right() - pRect.left() + 0.5
                                            : cRect.left() - pRect.right() - 0.5;
                player->setPos(player->x() + sepX, player->y());

                // 尝试推动木箱（只有玩家在地面时才能推）
                if (player->isOnGround) {
                    double crateMove = (pushDir > 0) ? qMax(pushSpeed, 3.0) : qMin(-pushSpeed, -3.0);
                    crate->setPos(crate->x() + crateMove, crate->y());
                    // 检查木箱是否撞墙
                    bool blocked = false;
                    { QRectF crateZ2 = crate->sceneBoundingRect().adjusted(-10,-10,10,10);
                    for (Tile* tile : floors) {
                        if (!tile->sceneBoundingRect().intersects(crateZ2)) continue;
                        if (crate->collidesWithItem(tile)) {
                            blocked = true;
                            break;
                        }
                    }
                    }
                    if (blocked) {
                        // 木箱推不动，玩家也不移动
                        crate->setPos(crate->x() - crateMove, crate->y());
                        player->vx = 0;
                    } else {
                        if (!cratePushSoundPlayed) {
                            sfxPlayer->setSource(QUrl("qrc:///tu/music/stone_push_medium.wav"));
                            sfxAudio->setVolume(0.5);
                            sfxPlayer->play();
                            cratePushSoundPlayed = true;
                        }
                    }
                }
                player->vx = 0; // 防止持续穿透
                break; // 每帧只处理一个木箱
            }
        }
    }

    // ====== 闪电形态：生成与更新尾气系统 ======
    // 保险机制：如果受到攻击丢失了形态或按T取消了形态，强制坠机
    if (player->currentForm != Enemy::SPARK) {
        player->isLightningFlying = false;
        player->endLightningDash();
    }

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
        exhaustItems[i]->setOpacity(exhaustLifetimes[i] / 15.0);
        if (exhaustLifetimes[i] <= 0) {
            scene->removeItem(exhaustItems[i]);
            delete exhaustItems[i];
            exhaustItems.removeAt(i);
            exhaustLifetimes.removeAt(i);
        }
    }
}

void MainWindow::mousePressEvent(QMouseEvent *event) { Q_UNUSED(event); }
void MainWindow::mouseMoveEvent(QMouseEvent *event) { Q_UNUSED(event); }

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    // UI点击音效
    auto playPop = [this]() {
        sfxPlayer->setSource(QUrl("qrc:///tu/music/pop_1.wav"));
        sfxAudio->setVolume(0.5);
        sfxPlayer->play();
    };

    if (obj == view) {
        if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            QPointF scenePos = view->mapToScene(me->pos());

            // === 主菜单悬停/点击 ===
            if (currentState == MAIN_MENU) {
                for (int i = 0; i < mainMenuBtnRects.size() && i < mainMenuButtons.size(); i++) {
                    bool inside = mainMenuBtnRects[i].contains(scenePos);
                    QColor normal, hover;
                    if (i == 0) {
                        normal = QColor(30, 50, 90, 210); hover = QColor(50, 80, 150, 230);
                    } else if (i == mainMenuBtnCount - 1) {
                        normal = QColor(80, 25, 25, 210); hover = QColor(160, 50, 50, 230);
                    } else {
                        normal = QColor(60, 60, 60, 210); hover = QColor(100, 100, 100, 230);
                    }
                    mainMenuButtons[i]->setBrush(inside ? QBrush(hover) : QBrush(normal));
                }
                if (event->type() == QEvent::MouseButtonPress &&
                    static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                    for (int i = 0; i < mainMenuBtnRects.size(); i++) {
                        if (mainMenuBtnRects[i].contains(scenePos)) {
                            playPop();
                            bool hasContinue = (savedLevelNum > 0);
                            if (i == 0) {
                                pendingAction = ACT_SHOW_LEVEL_SELECT;
                                return true;
                            } else if (hasContinue && i == 1) {
                                pendingAction = ACT_CONTINUE_GAME;
                                return true;
                            } else if ((hasContinue && i == 2) || (!hasContinue && i == 1)) {
                                pendingAction = ACT_SHOW_SETTINGS;
                                return true;
                            } else if (i == mainMenuBtnCount - 1) {
                                pendingAction = ACT_EXIT;
                                return true;
                            }
                        }
                    }
                }
            }

            // === 设置界面悬停/点击 ===
            if (currentState == SETTINGS) {
                // 返回按钮悬停
                if (!settingsBtnRects.isEmpty() && !settingsButtons.isEmpty()) {
                    bool inside = settingsBtnRects[0].contains(scenePos);
                    settingsButtons[0]->setBrush(inside ? QBrush(QColor(50, 80, 150, 230)) : QBrush(QColor(30, 50, 90, 210)));
                }
                // 音量滑块悬停效果
                if (settingsVolHandle && !settingsVolRect.isNull()) {
                    bool overVol = settingsVolRect.contains(scenePos);
                    settingsVolHandle->setBrush(overVol ? QBrush(QColor(255, 255, 255)) : QBrush(QColor(220, 220, 220)));
                    settingsVolHandle->setPen(QPen(overVol ? QColor(255, 255, 150) : Qt::white, overVol ? 2 : 1));
                }
                // 返回按钮点击
                if (event->type() == QEvent::MouseButtonPress &&
                    static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                    if (!settingsBtnRects.isEmpty() && settingsBtnRects[0].contains(scenePos)) {
                        playPop();
                        pendingAction = ACT_SHOW_MAIN_MENU;
                        return true;
                    }
                    // 音量滑块点击
                    if (!settingsVolRect.isNull() && settingsVolRect.contains(scenePos)) {
                        settingsDragging = true;
                        qreal relX = scenePos.x() - settingsVolRect.x();
                        if (relX < 0) relX = 0;
                        if (relX > settingsVolRect.width()) relX = settingsVolRect.width();
                        volumeLevel = relX / settingsVolRect.width();
                        audioOutput->setVolume(volumeLevel);
                        if (settingsVolFg) settingsVolFg->setRect(settingsVolRect.x(), settingsVolRect.y(),
                            settingsVolRect.width() * volumeLevel, settingsVolRect.height());
                        if (settingsVolHandle) settingsVolHandle->setPos(
                            settingsVolRect.x() + settingsVolRect.width() * volumeLevel - 7, settingsVolRect.y() - 4);
                        return true;
                    }
                }
            }
            if (event->type() == QEvent::MouseButtonRelease) {
                settingsDragging = false;
            }
            if (event->type() == QEvent::MouseMove && settingsDragging && currentState == SETTINGS) {
                qreal relX = scenePos.x() - settingsVolRect.x();
                if (relX < 0) relX = 0;
                if (relX > settingsVolRect.width()) relX = settingsVolRect.width();
                volumeLevel = relX / settingsVolRect.width();
                audioOutput->setVolume(volumeLevel);
                if (settingsVolFg) settingsVolFg->setRect(settingsVolRect.x(), settingsVolRect.y(),
                    settingsVolRect.width() * volumeLevel, settingsVolRect.height());
                if (settingsVolHandle) settingsVolHandle->setPos(
                    settingsVolRect.x() + settingsVolRect.width() * volumeLevel - 7, settingsVolRect.y() - 4);
            }

            // === 模式选择界面悬停/点击 ===
            if (currentState == LevelSelect) {
                if (isBossSelect) {
                    // ---- Boss选择子菜单 ----
                    for (int i = 0; i < bossCardRects.size() && i < bossCards.size(); i++) {
                        bool inside = bossCardRects[i].contains(scenePos);
                        QColor normal(50, 50, 80, 200);
                        QColor hover(80, 80, 160, 230);
                        if (bossCards[i]) {
                            bossCards[i]->setBrush(inside ? QBrush(hover) : QBrush(normal));
                            bossCards[i]->setPen(QPen(inside ? QColor(255, 255, 150) : QColor(150, 150, 200), inside ? 3 : 2));
                        }
                        if (inside && event->type() == QEvent::MouseButtonPress &&
                            static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                            playPop();
                            modeSelection = bossSelectTypes[i];
                            return true;
                        }
                    }
                    // 返回按钮（带悬停变色）
                    if (bossBackLabel && bossBackLabel->isVisible()) {
                        QRectF backRect = bossBackLabel->sceneBoundingRect().adjusted(-10, -5, 10, 5);
                        bool inside = backRect.contains(scenePos);
                        bossBackLabel->setDefaultTextColor(inside ? QColor(255, 255, 255) : QColor(180, 180, 255));
                        if (inside && event->type() == QEvent::MouseButtonPress &&
                            static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                            playPop();
                            showLevelSelect();
                            return true;
                        }
                    }
                } else {
                    for (int i = 0; i < cardRects.size() && i < levelCards.size(); i++) {
                        bool inside = cardRects[i].contains(scenePos);
                        QColor normal = (i == 0) ? QColor(30, 70, 30, 210) : QColor(70, 30, 30, 210);
                        QColor hover  = (i == 0) ? QColor(50, 140, 50, 230) : QColor(140, 50, 50, 230);
                        QPen normalPen((i == 0) ? QColor(100, 200, 100) : QColor(200, 80, 80), 2);
                        if (levelCards[i]) {
                            levelCards[i]->setBrush(inside ? QBrush(hover) : QBrush(normal));
                            levelCards[i]->setPen(QPen(inside ? QColor(255, 255, 150) : normalPen.color(), inside ? 3 : 2));
                        }
                        if (inside && event->type() == QEvent::MouseButtonPress &&
                            static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                            playPop();
                            if (cardLevelNums.value(i) == 2) {
                                // Boss战按钮 → 显示Boss选择子菜单
                                showBossSelect();
                            } else {
                                modeSelection = cardLevelNums.isEmpty() ? 1 : cardLevelNums[i];
                            }
                            return true;
                        }
                    }
                }
            }

            // === 暂停菜单悬停/点击 ===
            if (currentState == PAUSED) {
                for (int i = 0; i < pauseBtnRects.size() && i < pauseButtons.size(); i++) {
                    bool inside = pauseBtnRects[i].contains(scenePos);
                    QColor c1 = (i == 0) ? QColor(30, 80, 30, 200) : QColor(80, 30, 30, 200);
                    QColor c2 = (i == 0) ? QColor(50, 140, 50, 230) : QColor(140, 50, 50, 230);
                    pauseButtons[i]->setBrush(inside ? QBrush(c2) : QBrush(c1));
                }
                // 音量滑块悬停效果
                if (volHandle) {
                    bool overVol = volTrackRect.contains(scenePos);
                    volHandle->setBrush(overVol ? QBrush(QColor(255, 255, 255)) : QBrush(QColor(220, 220, 220)));
                    volHandle->setPen(QPen(overVol ? QColor(255, 255, 150) : Qt::white, overVol ? 2 : 1));
                }
                if (event->type() == QEvent::MouseButtonPress &&
                    static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                    for (int i = 0; i < pauseBtnRects.size(); i++) {
                        if (pauseBtnRects[i].contains(scenePos)) {
                            playPop();
                            if (i == 0) {
                                pendingAction = ACT_RESUME_GAME;
                                return true;
                            } else if (i == 1) {
                                pendingAction = ACT_SHOW_MAIN_MENU;
                                return true;
                            }
                        }
                    }
                    if (volTrackRect.contains(scenePos)) {
                        isDraggingVolume = true;
                        qreal relX = scenePos.x() - volTrackRect.x();
                        if (relX < 0) relX = 0;
                        if (relX > volTrackRect.width()) relX = volTrackRect.width();
                        volumeLevel = relX / volTrackRect.width();
                        audioOutput->setVolume(volumeLevel);
                        if (volTrackFg) volTrackFg->setRect(volTrackRect.x(), volTrackRect.y(),
                            volTrackRect.width() * volumeLevel, volTrackRect.height());
                        if (volHandle) volHandle->setPos(volTrackRect.x() + volTrackRect.width() * volumeLevel - 6, volTrackRect.y() - 4);
                        return true;
                    }
                }
                if (event->type() == QEvent::MouseButtonRelease) {
                    isDraggingVolume = false;
                }
                if (event->type() == QEvent::MouseMove && isDraggingVolume) {
                    qreal relX = scenePos.x() - volTrackRect.x();
                    if (relX < 0) relX = 0;
                    if (relX > volTrackRect.width()) relX = volTrackRect.width();
                    volumeLevel = relX / volTrackRect.width();
                    audioOutput->setVolume(volumeLevel);
                    if (volTrackFg) volTrackFg->setRect(volTrackRect.x(), volTrackRect.y(),
                        volTrackRect.width() * volumeLevel, volTrackRect.height());
                    if (volHandle) volHandle->setPos(volTrackRect.x() + volTrackRect.width() * volumeLevel - 6, volTrackRect.y() - 4);
                }
            }

            // === 游戏结束界面悬停/点击 ===
            if (currentState == GAME_OVER) {
                for (int i = 0; i < gameOverBtnRects.size() && i < gameOverButtons.size(); i++) {
                    bool inside = gameOverBtnRects[i].contains(scenePos);
                    QColor c1 = (i == 0) ? QColor(30, 80, 30, 210) : QColor(80, 25, 25, 210);
                    QColor c2 = (i == 0) ? QColor(50, 140, 50, 230) : QColor(140, 50, 50, 230);
                    gameOverButtons[i]->setBrush(inside ? QBrush(c2) : QBrush(c1));
                }
                if (event->type() == QEvent::MouseButtonPress &&
                    static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                    for (int i = 0; i < gameOverBtnRects.size(); i++) {
                        if (gameOverBtnRects[i].contains(scenePos)) {
                            playPop();
                            if (i == 0) {
                                pendingAction = ACT_RESTART_CHECKPOINT;
                                return true;
                            } else if (i == 1) {
                                pendingAction = ACT_SHOW_MAIN_MENU;
                                return true;
                            }
                        }
                    }
                }
            }

            // === 结束动画按钮悬停/点击 ===
            if (currentState == ENDING && endingStage >= 2 && endingMenuBtn) {
                bool inside = endingMenuBtn->sceneBoundingRect().contains(scenePos);
                QColor normal(30, 50, 90, 210), hover(50, 80, 150, 230);
                endingMenuBtn->setBrush(inside ? QBrush(hover) : QBrush(normal));
                if (event->type() == QEvent::MouseButtonPress &&
                    static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton && inside) {
                    playPop();
                    cleanupEndingUI();
                    cleanupGameObjects();
                    showMainMenu();
                    return true;
                }
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::cleanupSelectUI() {
    if (selectOverlay) { scene->removeItem(selectOverlay); delete selectOverlay; selectOverlay = nullptr; }
    if (selectTitle) { scene->removeItem(selectTitle); delete selectTitle; selectTitle = nullptr; }
    for (auto* c : levelCards) { scene->removeItem(c); delete c; }
    for (auto* l : cardLabels) { scene->removeItem(l); delete l; }
    for (auto* h : categoryHeaders) { scene->removeItem(h); delete h; }
    levelCards.clear(); cardLabels.clear(); cardRects.clear(); cardLevelNums.clear(); categoryHeaders.clear();
    if (menuText) { scene->removeItem(menuText); delete menuText; menuText = nullptr; }
    // 清理 Boss 选择子菜单
    for (auto* c : bossCards) { scene->removeItem(c); delete c; }
    for (auto* l : bossCardLabels) { scene->removeItem(l); delete l; }
    for (auto* img : bossCardImages) { scene->removeItem(img); delete img; }
    bossCards.clear(); bossCardLabels.clear(); bossCardImages.clear(); bossCardRects.clear(); bossSelectTypes.clear();
    if (bossBackLabel) { scene->removeItem(bossBackLabel); delete bossBackLabel; bossBackLabel = nullptr; }
    isBossSelect = false;
}

void MainWindow::cleanupPauseUI() {
    if (pauseOverlay) { scene->removeItem(pauseOverlay); delete pauseOverlay; pauseOverlay = nullptr; }
    if (pauseTitle) { scene->removeItem(pauseTitle); delete pauseTitle; pauseTitle = nullptr; }
    for (auto* b : pauseButtons) { scene->removeItem(b); delete b; }
    for (auto* t : pauseButtonTexts) { scene->removeItem(t); delete t; }
    pauseButtons.clear(); pauseButtonTexts.clear(); pauseBtnRects.clear();
    if (volTrackBg) { scene->removeItem(volTrackBg); delete volTrackBg; volTrackBg = nullptr; }
    if (volTrackFg) { scene->removeItem(volTrackFg); delete volTrackFg; volTrackFg = nullptr; }
    if (volHandle) { scene->removeItem(volHandle); delete volHandle; volHandle = nullptr; }
    isDraggingVolume = false;
}

void MainWindow::cleanupEndingUI() {
    if (endingOverlay) { scene->removeItem(endingOverlay); delete endingOverlay; endingOverlay = nullptr; }
    if (endingBlackBg) { scene->removeItem(endingBlackBg); delete endingBlackBg; endingBlackBg = nullptr; }
    if (endingBg) { scene->removeItem(endingBg); delete endingBg; endingBg = nullptr; }
    if (endingRock) { scene->removeItem(endingRock); delete endingRock; endingRock = nullptr; }
    if (endingKirby) { scene->removeItem(endingKirby); delete endingKirby; endingKirby = nullptr; }
    if (endingBigBird) { scene->removeItem(endingBigBird); delete endingBigBird; endingBigBird = nullptr; }
    if (endingSmallBird) { scene->removeItem(endingSmallBird); delete endingSmallBird; endingSmallBird = nullptr; }
    if (endingScreen) { scene->removeItem(endingScreen); delete endingScreen; endingScreen = nullptr; }
    if (endingMenuBtn) { scene->removeItem(endingMenuBtn); delete endingMenuBtn; endingMenuBtn = nullptr; }
    if (endingMenuBtnText) { scene->removeItem(endingMenuBtnText); delete endingMenuBtnText; endingMenuBtnText = nullptr; }
    endingKirbyFrames.clear();
    endingBigBirdFrames.clear();
    endingSmallBirdFrames.clear();
    endingAnimTimer = 0;
    endingStage = 0;
}

void MainWindow::startEndingAnimation() {
    currentState = ENDING;
    keys.clear();

    // 停止脚步和水声
    if (stepPlayer->playbackState() == QMediaPlayer::PlayingState) stepPlayer->stop();
    if (waterPlayer->playbackState() == QMediaPlayer::PlayingState) waterPlayer->stop();
    // 过关音效（暂停BGM）
    bgmPlayer->pause();
    bgmPaused = true;
    cinematicPlayer->setSource(QUrl("qrc:///tu/music/harpsichord_level_complete.wav"));
    cinematicAudio->setVolume(0.5);
    cinematicPlayer->play();

    // 隐藏 HUD
    staminaBar->setVisible(false);
    for (auto icon : lifeIcons) icon->setVisible(false);
    if (cooldownText) cooldownText->setVisible(false);
    if (fireCdIcon) fireCdIcon->setVisible(false);
    if (iceCdIcon)  iceCdIcon->setVisible(false);
    if (leafCdIcon) leafCdIcon->setVisible(false);
    if (bossHpBarBg) bossHpBarBg->setVisible(false);
    if (bossHpBarFg) bossHpBarFg->setVisible(false);

    // 锁定相机中心点
    endingBaseX = player->x();
    endingBaseY = 850;
    view->centerOn(endingBaseX, endingBaseY);

    // 用固定相机中心直接计算视口四角，确保始终居中不跟随角色
    qreal vw = view->viewport()->width();
    qreal vh = view->viewport()->height();
    qreal cx = endingBaseX - vw / 2.0;
    qreal cy = endingBaseY - vh / 2.0;

    auto load3Frames = [](const QString &path) -> QVector<QPixmap> {
        QVector<QPixmap> frames;
        QPixmap sheet(path);
        if (!sheet.isNull()) {
            int fh = sheet.height();
            int count = sheet.width() / fh;
            for (int i = 0; i < count && i < 3; i++)
                frames.push_back(sheet.copy(i * fh, 0, fh, fh));
        }
        return frames;
    };

    endingKirbyFrames = load3Frames(":/tu/jieshudonghua/kirby_huishou.png");
    endingBigBirdFrames = load3Frames(":/tu/jieshudonghua/bigbird.png");
    endingSmallBirdFrames = load3Frames(":/tu/jieshudonghua/smallbird.png");

    // 0. 黑底（使用成员变量，确保后续可清理）
    endingBlackBg = new QGraphicsRectItem(cx, cy, vw, vh);
    endingBlackBg->setBrush(Qt::black);
    endingBlackBg->setPen(Qt::NoPen);
    endingBlackBg->setZValue(2999);
    scene->addItem(endingBlackBg);

    // 1. 夕阳背景
    QPixmap sunsetPix(":/tu/jieshudonghua/sunset.png");
    if (!sunsetPix.isNull()) {
        endingBg = new QGraphicsPixmapItem(sunsetPix.scaled(vw, vh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        endingBg->setPos(cx, cy);
        endingBg->setZValue(3000);
        scene->addItem(endingBg);
    }

    // 2. 石头（左下角）
    QPixmap rockPix(":/tu/jieshudonghua/rock.png");
    qreal rh = 0;
    if (!rockPix.isNull()) {
        qreal rw = 380;
        rh = rw * rockPix.height() / rockPix.width();
        endingRock = new QGraphicsPixmapItem(rockPix.scaled(rw, rh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        endingRock->setPos(cx - 20, cy + vh - rh + 30);
        endingRock->setZValue(3001);
        scene->addItem(endingRock);
    }

    // 3. 卡比
    if (!endingKirbyFrames.isEmpty()) {
        qreal kw = 130;
        qreal kh = 130;
        endingKirby = new QGraphicsPixmapItem(endingKirbyFrames[0].scaled(kw, kh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        endingKirby->setZValue(3002);
        endingKirby->setPos(cx + vw * 0.08, cy + vh - rh - 10);
        scene->addItem(endingKirby);
    }

    // 4. 鸟儿（初始在屏幕右侧外，准备飞入）
    if (!endingBigBirdFrames.isEmpty()) {
        endingBigBird = new QGraphicsPixmapItem(endingBigBirdFrames[0].scaled(70, 70, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        endingBigBird->setZValue(3003);
        endingBigBird->setPos(cx + vw + 50, cy + vh * 0.42 - 35);
        scene->addItem(endingBigBird);
    }

    if (!endingSmallBirdFrames.isEmpty()) {
        endingSmallBird = new QGraphicsPixmapItem(endingSmallBirdFrames[0].scaled(45, 45, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        endingSmallBird->setZValue(3004);
        endingSmallBird->setPos(cx + vw + 100, cy + vh * 0.42 + 5);
        scene->addItem(endingSmallBird);
    }

    endingAnimTimer = 0;
    endingStage = 0;
}
void MainWindow::cleanupGameObjects() {
    cleanupEndingUI();
    // 先恢复猪鲨的水转换，防止Tile指针悬空
    if (dukeFishron) {
        for (auto& rec : dukeFishron->activeWaterChanges) {
            rec.tile->changeType(rec.originalType, rec.originalPixmap);
            waters.removeOne(rec.tile);
            floors.append(rec.tile);
        }
        dukeFishron->activeWaterChanges.clear();
        dukeFishron->pendingWaterConversions.clear();
    }
    for (Enemy* t : enemies) { scene->removeItem(t); delete t; }
    enemies.clear();
    for (Tile* t : waters) { scene->removeItem(t); delete t; }
    waters.clear();
    for (Tile* t : floors) { scene->removeItem(t); delete t; }
    floors.clear();
    spikes.clear();
    for (Cake* c : cakes) { scene->removeItem(c); delete c; }
    cakes.clear();
    for (Star* s : stars) { scene->removeItem(s); delete s; }
    stars.clear();
    for (Checkpoint* cp : checkpoints) { scene->removeItem(cp); delete cp; }
    checkpoints.clear();
    for (Goal* g : goals) { scene->removeItem(g); delete g; }
    goals.clear();
    for (Crate* crate : crates) { scene->removeItem(crate); delete crate; }
    crates.clear();
    for (auto* ex : exhaustItems) { scene->removeItem(ex); delete ex; }
    exhaustItems.clear(); exhaustLifetimes.clear();
    // 清理残留弹幕（防止重新进入游戏时访问悬空指针）
    for (auto* p : projectiles) { scene->removeItem(p); delete p; }
    projectiles.clear();
    // Boss已在enemies循环中delete，置空防止悬空指针
    cleanupTutorialTexts();
    dukeFishron = nullptr;
    brainOfCthulhu = nullptr;
    iceGod = nullptr;
    // 重置猪鲨二阶段背景
    for (QGraphicsRectItem* bg : phase2BgLayers) {
        bg->setOpacity(0.0);
    }
    for (QGraphicsRectItem* bg : backgroundLayers) {
        bg->setOpacity(1.0);
    }
    dukePhase2BgStarted = false;
    phase2BgFadeTimer = 0;
    // 清理登场动画残留
    if (bossIntroOverlay) { scene->removeItem(bossIntroOverlay); delete bossIntroOverlay; bossIntroOverlay = nullptr; }
    if (bossIntroExclamation) { scene->removeItem(bossIntroExclamation); delete bossIntroExclamation; bossIntroExclamation = nullptr; }
    bossIntroActive = false;
    bossIntroTimer = 0;
    bossSpawned = false;       // 重置，允许再次触发Boss战
    pendingBossType = 0;       // 清理Boss类型标记
    if (player) { scene->removeItem(player); delete player; player = nullptr; }
    staminaBar->setVisible(false);
    for (auto icon : lifeIcons) icon->setVisible(false);
    if (cooldownText) cooldownText->setVisible(false);
    if (fireCdIcon) fireCdIcon->setVisible(false);
    if (iceCdIcon)  iceCdIcon->setVisible(false);
    if (leafCdIcon) leafCdIcon->setVisible(false);
    if (bossHpBarBg) bossHpBarBg->setVisible(false);
    if (bossHpBarFg) bossHpBarFg->setVisible(false);
    // 停止脚步和水声
    if (stepPlayer->playbackState() == QMediaPlayer::PlayingState) stepPlayer->stop();
    if (waterPlayer->playbackState() == QMediaPlayer::PlayingState) waterPlayer->stop();
    // 恢复原始BGM源（退出Boss战后用），showMainMenu 会负责播放
    bgmPlayer->setSource(QUrl(originalBgmSource));
    bgmPlayer->setLoops(QMediaPlayer::Infinite);
    bgmPaused = true;  // 让 showMainMenu 恢复播放
}

// ====== 浮动文字教程系统实现 ======
void MainWindow::triggerTutorialText(TutorialTrigger* t) {
    if (!t || t->shown) return;
    t->shown = true;
    t->remainingFrames = t->durationFrames;

    // 移除旧文字
    for (auto& tt : tutorialTriggers) {
        if (tt.item) { scene->removeItem(tt.item); delete tt.item; tt.item = nullptr; }
    }

    t->item = new QGraphicsTextItem(t->message);
    QFont f = t->item->font();
    f.setFamily("SimHei");
    f.setPixelSize(16);
    f.setBold(true);
    t->item->setFont(f);
    t->item->setDefaultTextColor(QColor(255, 255, 200));
    t->item->setZValue(2500);

    qreal tx = qMax(player->x() - 180.0, 0.0);
    qreal ty = player->y() - 90.0;
    t->item->setPos(tx, ty);
    scene->addItem(t->item);
}

void MainWindow::updateTutorialTexts() {
    for (auto& t : tutorialTriggers) {
        if (!t.item) continue;
        t.remainingFrames--;
        if (t.remainingFrames <= 30) {
            t.item->setOpacity(qMax(t.remainingFrames / 30.0, 0.0));
        }
        if (t.remainingFrames <= 0) {
            scene->removeItem(t.item);
            delete t.item;
            t.item = nullptr;
        }
    }
}

void MainWindow::cleanupTutorialTexts() {
    for (auto& t : tutorialTriggers) {
        if (t.item) { scene->removeItem(t.item); delete t.item; t.item = nullptr; }
    }
    tutorialTriggers.clear();
}

void MainWindow::cleanupGameOverUI() {
    if (gameOverOverlay) { scene->removeItem(gameOverOverlay); delete gameOverOverlay; gameOverOverlay = nullptr; }
    if (gameOverTitle) { scene->removeItem(gameOverTitle); delete gameOverTitle; gameOverTitle = nullptr; }
    for (auto* b : gameOverButtons) { scene->removeItem(b); delete b; }
    for (auto* t : gameOverButtonTexts) { scene->removeItem(t); delete t; }
    gameOverButtons.clear();
    gameOverButtonTexts.clear();
    gameOverBtnRects.clear();
}

void MainWindow::showGameOver() {
    cleanupGameOverUI();

    qreal camX = player->x();
    qreal camY = 850;
    qreal cx = camX - SCREEN_W / 2.0;
    qreal cy = camY - SCREEN_H / 2.0;

    gameOverOverlay = new QGraphicsRectItem(cx, cy, SCREEN_W, SCREEN_H);
    gameOverOverlay->setBrush(QBrush(QColor(0, 0, 0, 180)));
    gameOverOverlay->setPen(Qt::NoPen);
    gameOverOverlay->setZValue(1999);
    scene->addItem(gameOverOverlay);

    gameOverTitle = new QGraphicsTextItem("GAME OVER");
    gameOverTitle->setFont(QFont("SimHei", 48, QFont::Bold));
    gameOverTitle->setDefaultTextColor(QColor(255, 80, 80));
    gameOverTitle->setZValue(2000);
    double tw = gameOverTitle->boundingRect().width();
    gameOverTitle->setPos(cx + (SCREEN_W - tw) / 2.0, cy + 80);
    scene->addItem(gameOverTitle);

    qreal btnW = 260, btnH = 50, btnSpacing = 20;
    qreal btnX = cx + (SCREEN_W - btnW) / 2.0;
    qreal btnY = cy + SCREEN_H / 2.0 + 20;

    struct { QString text; QColor bg; QColor border; } btns[] = {
        {"重新开始", QColor(30, 80, 30, 210), QColor(100, 200, 100)},
        {"返回主菜单", QColor(80, 25, 25, 210), QColor(200, 90, 90)},
    };

    gameOverButtons.clear();
    gameOverButtonTexts.clear();
    gameOverBtnRects.clear();

    for (int i = 0; i < 2; i++) {
        QRectF br(btnX, btnY + i * (btnH + btnSpacing), btnW, btnH);
        gameOverBtnRects.append(br);

        QGraphicsRectItem* btn = new QGraphicsRectItem(br);
        btn->setBrush(QBrush(btns[i].bg));
        btn->setPen(QPen(btns[i].border, 2));
        btn->setZValue(2000);
        scene->addItem(btn);
        gameOverButtons.append(btn);

        QGraphicsTextItem* txt = new QGraphicsTextItem(btns[i].text);
        txt->setFont(QFont("SimHei", 18, QFont::Bold));
        txt->setDefaultTextColor(Qt::white);
        txt->setZValue(2001);
        double tw2 = txt->boundingRect().width();
        txt->setPos(btnX + (btnW - tw2) / 2.0, btnY + i * (btnH + btnSpacing) + 10);
        scene->addItem(txt);
        gameOverButtonTexts.append(txt);
    }

    currentState = GAME_OVER;
    timer->start(16);
}

void MainWindow::cleanupMainMenuUI() {
    cleanupEndingUI();
    if (mainMenuBg0) { scene->removeItem(mainMenuBg0); delete mainMenuBg0; mainMenuBg0 = nullptr; }
    if (mainMenuBg1) { scene->removeItem(mainMenuBg1); delete mainMenuBg1; mainMenuBg1 = nullptr; }
    if (mainMenuOverlay) { scene->removeItem(mainMenuOverlay); delete mainMenuOverlay; mainMenuOverlay = nullptr; }
    if (mainMenuTitle) { scene->removeItem(mainMenuTitle); delete mainMenuTitle; mainMenuTitle = nullptr; }
    for (auto* b : mainMenuButtons) { scene->removeItem(b); delete b; }
    for (auto* t : mainMenuBtnTexts) { scene->removeItem(t); delete t; }
    mainMenuButtons.clear(); mainMenuBtnTexts.clear(); mainMenuBtnRects.clear();
    mainMenuBtnCount = 0;
}

void MainWindow::cleanupSettingsUI() {
    if (settingsOverlay) { scene->removeItem(settingsOverlay); delete settingsOverlay; settingsOverlay = nullptr; }
    if (settingsTitle) { scene->removeItem(settingsTitle); delete settingsTitle; settingsTitle = nullptr; }
    for (auto* b : settingsButtons) { scene->removeItem(b); delete b; }
    for (auto* t : settingsBtnTexts) { scene->removeItem(t); delete t; }
    settingsButtons.clear(); settingsBtnTexts.clear(); settingsBtnRects.clear();
    if (settingsVolBg) { scene->removeItem(settingsVolBg); delete settingsVolBg; settingsVolBg = nullptr; }
    if (settingsVolFg) { scene->removeItem(settingsVolFg); delete settingsVolFg; settingsVolFg = nullptr; }
    if (settingsVolHandle) { scene->removeItem(settingsVolHandle); delete settingsVolHandle; settingsVolHandle = nullptr; }
    settingsDragging = false;
}

void MainWindow::showMainMenu() {
    // 如果BGM被剧情音效暂停了，恢复BGM并停止剧情音效
    if (bgmPaused) {
        cinematicPlayer->stop();
        bgmPlayer->setSource(QUrl(originalBgmSource));
        bgmPlayer->setLoops(QMediaPlayer::Infinite);
        bgmPlayer->play();
        bgmPaused = false;
    }
    cleanupMainMenuUI();
    keys.clear(); // 清除残留按键，防止旧按键自动触发功能
    // 居中相机后获取实际可见区域
    view->centerOn(SCREEN_W / 2.0, SCREEN_H / 2.0);
    QPointF tl = view->mapToScene(0, 0);
    qreal cx = tl.x();
    qreal cy = tl.y();

    // 设置场景背景色，防止透明区域显示黑色
    scene->setBackgroundBrush(QColor(135, 206, 235)); // 天空蓝

    // 背景：kaishidonghua image.png 的两帧（各1000x700）重叠放置
    // frame1 叠在 frame0 上方（frame1 的透明区域可透出 frame0）
    QPixmap kaishiSheet(":/tu/kaishidonghua/image.png");
    if (!kaishiSheet.isNull() && kaishiSheet.width() >= 2000) {
        QPixmap frame0 = kaishiSheet.copy(0, 0, 1000, 700)
                              .scaled(SCREEN_W, SCREEN_H, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        QPixmap frame1 = kaishiSheet.copy(1000, 0, 1000, 700)
                              .scaled(SCREEN_W, SCREEN_H, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        mainMenuBg0 = new QGraphicsPixmapItem(frame0);
        mainMenuBg0->setPos(cx, cy);
        mainMenuBg0->setZValue(1998);
        scene->addItem(mainMenuBg0);

        mainMenuBg1 = new QGraphicsPixmapItem(frame1);
        mainMenuBg1->setPos(cx, cy);
        mainMenuBg1->setZValue(1999);
        scene->addItem(mainMenuBg1);
    }

    // 按钮列表
    struct BtnInfo { QString text; QColor bgColor; QColor borderColor; };
    QList<BtnInfo> btns;
    btns.append({"选择关卡", QColor(30, 50, 90, 210), QColor(80, 120, 220)});
    if (savedLevelNum > 0)
        btns.append({"继续游戏", QColor(30, 80, 30, 210), QColor(100, 200, 100)});
    btns.append({"设置", QColor(70, 60, 20, 210), QColor(180, 160, 60)});
    btns.append({"退出游戏", QColor(80, 25, 25, 210), QColor(200, 90, 90)});
    mainMenuBtnCount = btns.size();

    qreal btnW = 240, btnH = 50, btnSpacing = 18;
    qreal btnX = cx + (SCREEN_W - btnW) / 2.0;
    // 按钮在卡比图标正下方（屏幕高度约58%位置）
    qreal btnY = cy + SCREEN_H * 0.58;
    mainMenuButtons.clear(); mainMenuBtnTexts.clear(); mainMenuBtnRects.clear();

    for (int i = 0; i < btns.size(); i++) {
        QRectF br(btnX, btnY + i * (btnH + btnSpacing), btnW, btnH);
        mainMenuBtnRects.append(br);

        QGraphicsRectItem* btn = new QGraphicsRectItem(br);
        btn->setBrush(QBrush(btns[i].bgColor));
        btn->setPen(QPen(btns[i].borderColor, 2));
        btn->setZValue(2000);
        scene->addItem(btn);
        mainMenuButtons.append(btn);

        QGraphicsTextItem* txt = new QGraphicsTextItem(btns[i].text);
        txt->setFont(QFont("SimHei", 18, QFont::Bold));
        txt->setDefaultTextColor(Qt::white);
        txt->setZValue(2001);
        double tw = txt->boundingRect().width();
        txt->setPos(btnX + (btnW - tw) / 2.0, btnY + i * (btnH + btnSpacing) + 8);
        scene->addItem(txt);
        mainMenuBtnTexts.append(txt);
    }

    for (QGraphicsRectItem* bg : backgroundLayers) {
        bg->setPos(500 - bg->rect().width() / 2.0, 0);
    }
    currentState = MAIN_MENU;
    timer->start(16);
}

void MainWindow::showSettings() {
    cleanupSettingsUI();
    keys.clear();
    // 居中相机后获取实际可见区域
    view->centerOn(SCREEN_W / 2.0, SCREEN_H / 2.0);
    QPointF tl = view->mapToScene(0, 0);
    qreal cx = tl.x();
    qreal cy = tl.y();

    settingsOverlay = new QGraphicsRectItem(cx, cy, SCREEN_W, SCREEN_H);
    settingsOverlay->setBrush(QBrush(QColor(0, 0, 0, 180)));
    settingsOverlay->setPen(Qt::NoPen);
    settingsOverlay->setZValue(1999);
    scene->addItem(settingsOverlay);

    settingsTitle = new QGraphicsTextItem("设置");
    settingsTitle->setFont(QFont("SimHei", 36, QFont::Bold));
    settingsTitle->setDefaultTextColor(QColor(255, 255, 100));
    settingsTitle->setZValue(2000);
    settingsTitle->setPos(cx + (SCREEN_W - settingsTitle->boundingRect().width()) / 2.0, cy + 40);
    scene->addItem(settingsTitle);

    qreal btnW = 260, btnH = 50;
    qreal btnX = cx + (SCREEN_W - btnW) / 2.0;
    qreal curY = cy + 130;

    // 音量标签 + 滑块
    QGraphicsTextItem* volLabel = new QGraphicsTextItem("音量");
    volLabel->setFont(QFont("SimHei", 16, QFont::Bold));
    volLabel->setDefaultTextColor(Qt::white);
    volLabel->setZValue(2001);
    volLabel->setPos(btnX, curY);
    scene->addItem(volLabel);
    settingsBtnTexts.append(volLabel);

    curY += 30;
    qreal trackW = btnW, trackH = 24;
    settingsVolRect = QRectF(btnX, curY, trackW, trackH);
    settingsVolBg = new QGraphicsRectItem(settingsVolRect);
    settingsVolBg->setBrush(QBrush(QColor(50, 50, 50)));
    settingsVolBg->setPen(QPen(QColor(120, 120, 120), 1));
    settingsVolBg->setZValue(2000);
    scene->addItem(settingsVolBg);

    settingsVolFg = new QGraphicsRectItem(btnX, curY, trackW * volumeLevel, trackH);
    settingsVolFg->setBrush(QBrush(QColor(80, 160, 255)));
    settingsVolFg->setPen(Qt::NoPen);
    settingsVolFg->setZValue(2001);
    scene->addItem(settingsVolFg);

    settingsVolHandle = new QGraphicsRectItem(btnX + trackW * volumeLevel - 5, curY - 4, 14, trackH + 8);
    settingsVolHandle->setBrush(QBrush(QColor(220, 220, 220)));
    settingsVolHandle->setPen(QPen(Qt::white, 1));
    settingsVolHandle->setZValue(2002);
    scene->addItem(settingsVolHandle);

    // 返回按钮
    curY += trackH + 30;
    QRectF backRect(btnX, curY, btnW, btnH);
    settingsBtnRects.append(backRect);
    QGraphicsRectItem* backBtn = new QGraphicsRectItem(backRect);
    backBtn->setBrush(QBrush(QColor(30, 50, 90, 210)));
    backBtn->setPen(QPen(QColor(80, 120, 220), 2));
    backBtn->setZValue(2000);
    scene->addItem(backBtn);
    settingsButtons.append(backBtn);

    QGraphicsTextItem* backTxt = new QGraphicsTextItem("返回主菜单");
    backTxt->setFont(QFont("SimHei", 18, QFont::Bold));
    backTxt->setDefaultTextColor(Qt::white);
    backTxt->setZValue(2001);
    double tw = backTxt->boundingRect().width();
    backTxt->setPos(btnX + (btnW - tw) / 2.0, curY + 8);
    scene->addItem(backTxt);
    settingsBtnTexts.append(backTxt);

    for (QGraphicsRectItem* bg : backgroundLayers) {
        bg->setPos(500 - bg->rect().width() / 2.0, 0);
    }
    currentState = SETTINGS;
}

void MainWindow::showLevelSelect() {
    cleanupSelectUI();
    keys.clear(); // 清除残留按键，防止旧按键自动触发
    modeSelection = 0;
    isBossSelect = false;
    // 居中相机后获取实际可见区域
    view->centerOn(SCREEN_W / 2.0, SCREEN_H / 2.0);
    QPointF tl = view->mapToScene(0, 0);
    qreal cx = tl.x();
    qreal cy = tl.y();

    selectOverlay = new QGraphicsRectItem(cx, cy, SCREEN_W, SCREEN_H);
    selectOverlay->setBrush(QBrush(QColor(0, 0, 0, 180)));
    selectOverlay->setPen(Qt::NoPen);
    selectOverlay->setZValue(1999);
    scene->addItem(selectOverlay);

    selectTitle = new QGraphicsTextItem("选择模式");
    selectTitle->setFont(QFont("SimHei", 36, QFont::Bold));
    selectTitle->setDefaultTextColor(QColor(255, 255, 100));
    selectTitle->setZValue(2000);
    selectTitle->setPos(cx + (SCREEN_W - selectTitle->boundingRect().width()) / 2.0, cy + 40);
    scene->addItem(selectTitle);

    // ====== 两个模式按钮：冒险模式 & Boss战 ======
    struct ModeInfo { QString name; QString desc; QColor bg; QColor border; QColor hover; };
    ModeInfo modes[] = {
        {"冒险模式", "体验星之卡比的经典冒险", QColor(30, 70, 30, 210), QColor(100, 200, 100), QColor(50, 140, 50, 230)},
        {"Boss战",   "挑战强大的Boss",          QColor(70, 30, 30, 210), QColor(200, 80, 80),   QColor(140, 50, 50, 230)},
    };

    cardRects.clear(); cardLevelNums.clear();
    qreal btnW = 280, btnH = 80, btnSpacing = 30;
    qreal totalH = 2 * btnH + btnSpacing;
    qreal startY = cy + (SCREEN_H - totalH) / 2.0 + 20;
    qreal btnX = cx + (SCREEN_W - btnW) / 2.0;

    for (int i = 0; i < 2; i++) {
        qreal curY = startY + i * (btnH + btnSpacing);
        QRectF br(btnX, curY, btnW, btnH);
        cardRects.append(br);
        cardLevelNums.append(i + 1); // 1=冒险, 2=Boss

        QGraphicsRectItem* card = new QGraphicsRectItem(br);
        card->setBrush(QBrush(modes[i].bg));
        card->setPen(QPen(modes[i].border, 2));
        card->setZValue(2000);
        scene->addItem(card);
        levelCards.append(card);

        // 模式名称
        QGraphicsTextItem* label = new QGraphicsTextItem(modes[i].name);
        label->setFont(QFont("SimHei", 20, QFont::Bold));
        label->setDefaultTextColor(Qt::white);
        label->setZValue(2001);
        double lw = label->boundingRect().width();
        label->setPos(btnX + (btnW - lw) / 2.0, curY + 12);
        scene->addItem(label);
        cardLabels.append(label);

        // 模式描述
        QGraphicsTextItem* desc = new QGraphicsTextItem(modes[i].desc);
        desc->setFont(QFont("SimHei", 11));
        desc->setDefaultTextColor(QColor(200, 200, 200));
        desc->setZValue(2001);
        double dw = desc->boundingRect().width();
        desc->setPos(btnX + (btnW - dw) / 2.0, curY + btnH - 24);
        scene->addItem(desc);
        cardLabels.append(desc);
    }

    // 底部提示
    menuText = new QGraphicsTextItem("- 按 ESC 返回主菜单 -");
    menuText->setFont(QFont("SimHei", 13, QFont::Bold));
    menuText->setDefaultTextColor(QColor(200, 200, 200));
    menuText->setZValue(2000);
    double tw = menuText->boundingRect().width();
    menuText->setPos(cx + (SCREEN_W - tw) / 2.0, startY + 2 * (btnH + btnSpacing) + 10);
    scene->addItem(menuText);

    for (QGraphicsRectItem* bg : backgroundLayers) {
        bg->setPos(500 - bg->rect().width() / 2.0, 0);
    }
    currentState = LevelSelect;
    timer->start(16);
}

void MainWindow::showBossSelect() {
    // 保留 overlay，移除之前的模式按钮和文字
    for (auto* c : levelCards) { scene->removeItem(c); delete c; }
    levelCards.clear();
    for (auto* l : cardLabels) { scene->removeItem(l); delete l; }
    cardLabels.clear();
    cardRects.clear();
    cardLevelNums.clear();
    if (menuText) { scene->removeItem(menuText); delete menuText; menuText = nullptr; }

    // 清理旧的 boss 选择 UI（防止残留）
    for (auto* c : bossCards) { scene->removeItem(c); delete c; }
    bossCards.clear();
    for (auto* l : bossCardLabels) { scene->removeItem(l); delete l; }
    bossCardLabels.clear();
    for (auto* img : bossCardImages) { scene->removeItem(img); delete img; }
    bossCardImages.clear();
    bossCardRects.clear();
    bossSelectTypes.clear();
    if (bossBackLabel) { scene->removeItem(bossBackLabel); delete bossBackLabel; bossBackLabel = nullptr; }

    isBossSelect = true;
    modeSelection = 0;

    view->centerOn(SCREEN_W / 2.0, SCREEN_H / 2.0);
    QPointF tl = view->mapToScene(0, 0);
    qreal cx = tl.x();
    qreal cy = tl.y();

    // 标题
    if (selectTitle) {
        selectTitle->setPlainText("选择Boss");
    }

    // 三个 Boss 信息
    struct BossInfo {
        QString name;
        QString desc;
        QString spritePath;
        int type;       // 1,2,3
        int frameCount; // 竖排帧数
    };
    BossInfo bosses[] = {
        {"克苏鲁之脑", "疯狂的紫色眼球", ":/tu/brainofcthlu.png", 1, 8},
        {"猪鲨公爵",   "凶猛的鲨鱼Boss",  ":/tu/pig_shark.png",  2, 8},
        {"冰雪之神",   "冰霜雪花守护者",  ":/tu/ice god.png",    3, 6},
    };

    // 布局：3张卡片水平排列
    int cardCount = 3;
    qreal cardW = 200, cardH = 280, spacing = 30;
    qreal totalW = cardCount * cardW + (cardCount - 1) * spacing;
    qreal startX = cx + (SCREEN_W - totalW) / 2.0;
    qreal startY = cy + (SCREEN_H - cardH) / 2.0 + 30;

    for (int i = 0; i < cardCount; i++) {
        qreal curX = startX + i * (cardW + spacing);
        QRectF br(curX, startY, cardW, cardH);
        bossCardRects.append(br);
        bossSelectTypes.append(20 + bosses[i].type); // 21,22,23

        // 卡片背景
        QGraphicsRectItem* card = new QGraphicsRectItem(br);
        card->setBrush(QBrush(QColor(50, 50, 80, 200)));
        card->setPen(QPen(QColor(150, 150, 200), 2));
        card->setZValue(2000);
        scene->addItem(card);
        bossCards.append(card);

        // Boss 图片（加载 sprite 的第一帧，缩放到合适大小）
        QPixmap sheet(bosses[i].spritePath);
        QPixmap bossPix;
        if (!sheet.isNull()) {
            // 取 sprite 第一帧（竖排多帧，取第一帧高度 = 总高 / 帧数）
            int frameH = sheet.height() / bosses[i].frameCount;
            bossPix = sheet.copy(0, 0, sheet.width(), frameH);
            bossPix = bossPix.scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else {
            // 兜底：彩色方块
            bossPix = QPixmap(160, 160);
            bossPix.fill(Qt::transparent);
            QPainter p(&bossPix);
            p.setBrush(QColor(100, 100, 200));
            p.setPen(Qt::NoPen);
            p.drawRect(0, 0, 160, 160);
            p.end();
        }

        QGraphicsPixmapItem* pImg = new QGraphicsPixmapItem(bossPix);
        pImg->setPos(curX + (cardW - bossPix.width()) / 2.0, startY + 15);
        pImg->setZValue(2001);
        scene->addItem(pImg);
        bossCardImages.append(pImg);

        // Boss 名称
        QGraphicsTextItem* nameLabel = new QGraphicsTextItem(bosses[i].name);
        nameLabel->setFont(QFont("SimHei", 18, QFont::Bold));
        nameLabel->setDefaultTextColor(Qt::white);
        nameLabel->setZValue(2001);
        double nw = nameLabel->boundingRect().width();
        nameLabel->setPos(curX + (cardW - nw) / 2.0, startY + cardH - 65);
        scene->addItem(nameLabel);
        bossCardLabels.append(nameLabel);

        // 描述
        QGraphicsTextItem* descLabel = new QGraphicsTextItem(bosses[i].desc);
        descLabel->setFont(QFont("SimHei", 11));
        descLabel->setDefaultTextColor(QColor(200, 200, 200));
        descLabel->setZValue(2001);
        double dw = descLabel->boundingRect().width();
        descLabel->setPos(curX + (cardW - dw) / 2.0, startY + cardH - 35);
        scene->addItem(descLabel);
        bossCardLabels.append(descLabel);
    }

    // 返回按钮
    bossBackLabel = new QGraphicsTextItem("← 返回");
    bossBackLabel->setFont(QFont("SimHei", 16, QFont::Bold));
    bossBackLabel->setDefaultTextColor(QColor(180, 180, 255));
    bossBackLabel->setZValue(2001);
    bossBackLabel->setPos(cx + 30, cy + SCREEN_H - 60);
    scene->addItem(bossBackLabel);
}
