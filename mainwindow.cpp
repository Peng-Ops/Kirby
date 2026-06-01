#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "basicenemy.h"
#include "apple.h"
#include <QPainter>
#include <QStyleFactory>
#include <QMouseEvent>
int originalSize = 24;
int renderSize = originalSize * 2;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    player = nullptr;
    this->resize(1000, 700);

    // 1. 场景
    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 5000, 1000); // 地图长度设为 5000

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

    // 初始复活位置 = 卡比出生点
    lastCheckpointPos = QPointF(800, 856);
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
    audioOutput->setVolume(volumeLevel); // 音量范围 0.0 到 1.0
    bgmPlayer->setLoops(QMediaPlayer::Infinite); // 无限循环
    bgmPlayer->play();

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

    // E. 清理上关遗留的蛋糕（防止遗漏内存泄漏）
    for (Cake* c : cakes) { scene->removeItem(c); delete c; }
    cakes.clear();

    // F. 清理上关遗留的检查点（防止遗漏内存泄漏）
    for (Checkpoint* cp : checkpoints) { scene->removeItem(cp); delete cp; }
    checkpoints.clear();

    // G. 清理上关遗留的终点
    for (Goal* g : goals) { scene->removeItem(g); delete g; }
    goals.clear();

    // H. 安全删除玩家
    if (player != nullptr) {
        scene->removeItem(player);
        delete player;
        player = nullptr; // 销毁后立刻置空
    }

    if (!scene) return; // 安全检查
    qreal sceneH = scene->sceneRect().height();

    // 2. 根据关卡数,选择不同的矩阵(6关,3类)
    QStringList levelData;
    if (levelNum == 1) { // 战斗: 星之绿地
        levelData = {
            ".................................................................66............................",
            "...........................................1111..................66............................",
            "...........................................2222.........I.........66.......................P....",
            "..............1111.........F..................22222......1111......66....................2222222",
            ".....P........2222.......1111.....G...........22222..L...22222.....66....................2222222",
            "....1111......22222......2222....1111............................22222....F..............2222222",
            "...P2222....222222......2222...P2222....I....1111................22222...1111............2222222",
            "...111111....2222222..G.......111111....1111...2222....F.........22222...2222...I........2222222",
            "11112222111122222221111111111122222111112222..22222...1111..L....22222..22222..1111.......2222222",
            "2222222222222222222222222222222222222222222111112222111222211111222222111222211222211111112222222",
            "2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222",
        };
    } else if (levelNum == 2) { // 跑酷: 刺之回廊
        levelData = {
            "..............................................................................................",
            "..............................................................................................",
            ".....1111...........S...............1111...................1111..................S..............",
            ".P...2222..S.......1111....S.........2222....S............2222....S............1111....S........",
            "....22222..1111....2222...1111......P2222...1111..........2222...1111...........2222...1111......",
            "...P2222...2222....2222...2222.......2222...2222...S......2222..P2222..S.......2222..P2222......",
            "....2222..P2222...2222...2222.......2222...2222..1111....2222..P2222..1111....2222..P2222..T....",
            "...22222........T........P2222..T.........P...T..2222..T..........P...T..2222............T.....",
            "...22222..T........T.........T...........T........2222.......T...........2222.......T..........",
            "..P2222......T........T................T............P............T...........P.......T.........",
            "..22222...........T...........T.............T............T..............T............T.........",
            "111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111.",
            "2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222",
        };
    } else if (levelNum == 3) { // 坑爹: 寒冰炼狱
        levelData = {
            "..............................................................................................",
            "...................................1111.......................................................",
            ".........................S.........2222...S..............55...................................",
            "........................1111.......22222..1111..........P5555..W..............................",
            "............W...........2222..............2222......E....5555.........X.......................",
            "...........1111.....S...2222..X...........2222.....1111..5555...S.....1111....................",
            "....P......2222....1111.22222.1111....W...2222.....2222..5555..1111....2222...........X.......",
            "...1111....222.2...2222.22222.2222...1111...........22222......P2222....2222....E.............",
            "...P2222...22222...22222....S..2222...2222..X.......22222..S....2222...P2222...1111...........",
            "..122221...222222...22222..11112222..12222..1111....22222.1111..12222...12222...P2222..E.......",
            ".11222221111222222111222221112222221112222111222211112222211222111222211112222111222221111......",
            "112222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222.",
            "2222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222",
        };
    } else if (levelNum == 4) { // 跑酷: 云中回廊
        levelData = {
            "..............................................................................................",
            ".........66.................66................66................................................",
            "...S.....66.......S.........66.......S........66........S.........1111.........S...............",
            "..1111...........1111................1111................1111.......2222......1111..............",
            "..P222..S........P222..S.....S.......P222..S.....S.......P222..S....22222.....P222...S...S.....",
            "..P222.1111......P222.1111..1111.....P222.1111..1111.....P222.1111...22222...T.P222..1111.1111..",
            "..P222.P222......P222.P222..P222.....P222.P222..P222.....P222.P222...P2222......P222..P222.P222.",
            "....T..22222..T....T..22222..22222.T....T..22222..22222.T....T..22222....T...T....T..22222..T...",
            "...T......P....T....T.....P....T....T....T.....P....T....T....T.....P.......T....T.....P.......",
            "..2222...........2222...........2222...........2222...........2222...........2222..............",
            ".12222111111111112222111111111112222111111111112222111111111112222111111111112222111111111111111",
            "1122222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222",
        };
    } else if (levelNum == 5) { // 战斗: 熔岩堡垒
        levelData = {
            "..............................................................................................",
            ".............1111.........................1111.........................1111...................",
            ".....P.......2222..F..........P............2222...F........P...........2222..F..........P......",
            "....1111.....22222.1111......1111.....F....22222.1111......1111....F...22222.1111......1111.....",
            "...P2222..I..77777.P2222....P2222....1111.....888.P2222....P2222...1111..777.P2222....P2222....",
            "...P2222.....77777..22222..P.2222.I..2222...........22222..P.2222.I.2222........P2222..P.2222...",
            "...12222..F..77777.12222....12222....2222..F........12222...12222....2222..F.......12222.L......",
            "...12222111112222221222211111222211111222211111111111222211111222211111222211111111112222111111..",
            "..P22222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222.",
            "..1222211111111111111111111111111111111111111111111111111111111111111111111111111111111111111111.",
            "1122222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222",
        };
    } else if (levelNum == 6) { // 坑爹: 暗影洞穴
        levelData = {
            "..............................................................................................",
            "..................................1111.......................................................",
            "..................X............E..2222...X...........1111.........................X..........",
            ".................1111..............22222..1111........2222....E...............W..............",
            ".........W.......2222...X....E.....22222..2222...E....22222..1111......X...1111..............",
            "........1111.....2222..1111.......P2222..P2222..1111....2222..2222.....1111.2222......E......",
            ".P......P2222....P2222.P2222.......2222...222...P2222..P2222..P2222....P2222........1111.....",
            "1122....12222....12222.12222..X...12222..12222..12222..12222..12222...P12222.X......P2222.....",
            "2222....22222....22222.22222.1111..22222..22222..22222..22222..22222....22222.1111..P2222..X..",
            "2222....22222....22222.22222.2222..2222...22222..22222..22222..22222....22222.2222..P2222.1111",
            "22221111222221111222221122221122221122221112222111222221122221122221111222221122211112222112222",
            "222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222",
        };
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
            // ====== Apple 追踪苹果 E ======
            else if (type == 'E') {
                Apple* apple = new Apple();
                apple->setPos(c * renderSize, r * renderSize + bottomOffset);
                apple->setScale(1.0);
                scene->addItem(apple);
                enemies.append(apple);
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
            // ====== 伏击刺 X ======
            else if (type == 'X')  tile = new Tile(Tile::AmbushSpike, ciPix);
            else if (type == 'C'){
                Cake* cake = new Cake();
                cake->setPos(c * renderSize, r * renderSize + bottomOffset);
                //cake->setVisible(false);
                scene->addItem(cake);
                cakes.append(cake);
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
            // ====== 移动刺 S/T/U/W ======
            else if (type == 'S') {
                Tile* tile = new Tile(Tile::Spike, ciPix);
                tile->setPos(c * renderSize, r * renderSize + bottomOffset);
                tile->setScale(2.0);
                tile->enableMove(Tile::Horizontal, 2.0, 96);
                scene->addItem(tile);
                floors.append(tile); spikes.append(tile);
                continue;
            }
            else if (type == 'T') {
                Tile* tile = new Tile(Tile::Spike, ci2Pix);
                tile->setPos(c * renderSize, r * renderSize + bottomOffset);
                tile->setScale(2.0);
                tile->enableMove(Tile::Vertical, 1.5, 72);
                scene->addItem(tile);
                floors.append(tile); spikes.append(tile);
                continue;
            }
            else if (type == 'U') {
                Tile* tile = new Tile(Tile::Spike, daociPix);
                tile->setPos(c * renderSize, r * renderSize + bottomOffset);
                tile->setScale(2.0);
                tile->enableMove(Tile::Horizontal, 2.5, 120);
                scene->addItem(tile);
                floors.append(tile); spikes.append(tile);
                continue;
            }
            else if (type == 'W') {
                Tile* tile = new Tile(Tile::Spike, qiangciPix);
                tile->setPos(c * renderSize, r * renderSize + bottomOffset);
                tile->setScale(2.0);
                tile->enableMove(Tile::Vertical, 2.0, 80);
                scene->addItem(tile);
                floors.append(tile); spikes.append(tile);
                continue;
            }

            if (tile) {
                tile->setPos(c * renderSize, r * renderSize + bottomOffset);
                tile->setScale(2.0);
                scene->addItem(tile);
                if (type == '1' || type == '2' || type == '5' || type == '6') floors.append(tile);
                else if (type == '3' || type == '4') waters.append(tile);
                if (type == '7' || type == '8' || type == '9' || type == 'A' || type == 'B' || type == 'X') {
                    floors.append(tile);  // 刺也能站上去（物理碰撞）
                    spikes.append(tile);  // 但对玩家造成伤害
                }
            }
        }
    }

    // Boss已在enemies循环中delete，置空防止悬空指针
    dukeFishron = nullptr;
    brainOfCthulhu = nullptr;
    iceGod = nullptr;

    // 4. 重生卡比（必须在Boss创建之前）
    player = new Player();
    player->setPos(800, bottomOffset - 100);
    scene->addItem(player);
    lastCheckpointPos = QPointF(800, bottomOffset - 100);

    // ====== Boss动态生成配置（战斗关卡Boss不在此创建，由gameUpdate检测位置后生成）======
    pendingBossType = 0;
    bossSpawnX = (levelData[0].length() - 10) * renderSize;
    bossSpawnY = 400;
    bossSpawned = false;

    if (levelNum == 1)      pendingBossType = 1; // 克苏鲁之脑
    else if (levelNum == 5) pendingBossType = 2; // 猪鲨
    // 跑酷(2,4)和坑爹(3,6)关卡无Boss

    // ====== 终点旗（关卡最右端） ======
    {
        int groundRow = levelData.size() - 2;
        qreal goalX = (levelData[0].length() - 5) * renderSize;
        Goal* goal = new Goal();
        goal->setPos(goalX, groundRow * renderSize + bottomOffset - 48);
        scene->addItem(goal);
        goals.append(goal);
    }

    // 5. 切换 UI 状态 (隐藏菜单字，显示 HUD)
    if (titleText) titleText->setVisible(false);
    if (hintText)  hintText->setVisible(false);
    staminaBar->setVisible(true);
    for (auto icon : lifeIcons) icon->setVisible(true);

    // 6. 状态机切换与启动
    currentState = PLAYING;
    timer->start(16);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    // ====== ESC暂停/恢复 ======
    if (event->key() == Qt::Key_Escape && !event->isAutoRepeat()) {
        if (currentState == PLAYING) {
            timer->stop();
            savedLevelNum = currentLevelNum;
            savedCheckpointPos = lastCheckpointPos;
            savedForm = player->currentForm;
            savedHP = player->hp;
            savedStamina = player->stamina;
            savedAttackPowerTimer = player->attackPowerTimer;

            qreal camX = player->x();
            qreal camY = 850;
            qreal vw = view->viewport()->width();
            qreal vh = view->viewport()->height();

            pauseOverlay = new QGraphicsRectItem(camX - vw/2.0, camY - vh/2.0, vw, vh);
            pauseOverlay->setBrush(QBrush(QColor(0, 0, 0, 170)));
            pauseOverlay->setPen(Qt::NoPen);
            pauseOverlay->setZValue(2000);
            scene->addItem(pauseOverlay);

            pauseTitle = new QGraphicsTextItem("游戏暂停");
            pauseTitle->setFont(QFont("SimHei", 40, QFont::Bold));
            pauseTitle->setDefaultTextColor(Qt::white);
            pauseTitle->setZValue(2001);
            double tw = pauseTitle->boundingRect().width();
            pauseTitle->setPos(camX - tw/2.0, camY - vh/3.0);
            scene->addItem(pauseTitle);

            qreal btnW = 260, btnH = 50;
            qreal btnX = camX - btnW/2.0;
            qreal btnY = camY - vh/6.0;

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
        } else if (currentState == PAUSED) {
            cleanupPauseUI();
            currentState = PLAYING;
            timer->start(16);
            return;
        }
    }

    if (currentState == START_SCREEN) {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            enterPressed = true;
        }
        return;
    }

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
}
void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    keys.remove(event->key());
    if (event->key() == lastHorizontalKey)
        lastHorizontalKey = 0;
}

void MainWindow::gameUpdate() {
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
            player->invulnTimer = 60;
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

    // 阶段一：静止的开始界面
    if (currentState == START_SCREEN) {
        if (enterPressed) {
            player->vx = 0;
            if (player->isOnGround){
                enterPressed = false;
                if (titleText) { scene->removeItem(titleText); delete titleText; titleText = nullptr; }
                if (hintText) { scene->removeItem(hintText); delete hintText; hintText = nullptr; }
                showMainMenu();
                return;
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

        if (player->y() >= 856) {
            player->setPos(player->x(), 856);
            player->vy = 0;
            player->isOnGround = true;
        }

        player->updateLogic();
        // 锁定镜头
        view->centerOn(500, 900);
        for (QGraphicsRectItem* bg : backgroundLayers) {
            bg->setPos(500 - bg->rect().width() / 2.0, 0);
        }
        return;
    }
    if (currentState == LevelSelect) {
        if (keys.contains(Qt::Key_Escape)) {
            keys.remove(Qt::Key_Escape);
            cleanupSelectUI();
            showMainMenu();
            return;
        }
        if (savedLevelNum > 0 && keys.contains(Qt::Key_0)) {
            keys.remove(Qt::Key_0);
            cleanupSelectUI();
            currentState = PLAYING;
            loadLevel(savedLevelNum);
            player->setPos(savedCheckpointPos.x(), savedCheckpointPos.y());
            player->hp = savedHP;
            player->stamina = savedStamina;
            player->currentForm = savedForm;
            player->attackPowerTimer = savedAttackPowerTimer;
            player->invulnTimer = 60;
            lastCheckpointPos = savedCheckpointPos;
            savedLevelNum = 0;
            return;
        }
        // 检查按键1-6选择关卡
        for (int lv = 1; lv <= 6; lv++) {
            if (keys.contains(static_cast<Qt::Key>(Qt::Key_1 + lv - 1))) {
                keys.remove(static_cast<Qt::Key>(Qt::Key_1 + lv - 1));
                currentState = PLAYING;
                cleanupSelectUI();
                loadLevel(lv);
                return;
            }
        }
        return;
    }

    if (currentState == MAIN_MENU) return;
    if (currentState == SETTINGS) return;
    if (currentState == GAME_OVER) return;
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

        // 4. 水下移动扣除体力
        if (keys.contains(Qt::Key_W) || keys.contains(Qt::Key_A) || keys.contains(Qt::Key_D)) {
            player->stamina-=1.5;
            if (player->stamina <= 0) {
                player->hp = 0;
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
    for (int i = floors.size() - 1; i >= 0; i--) {
        Tile *tile = floors[i];
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
            // 4. 核心逻辑：撞墙，或者前方是悬崖（没地了），立刻掉头
            QRectF eRect = enemy->sceneBoundingRect();
            for (Tile *tile : floors) {
                QRectF tRect = tile->sceneBoundingRect();
                if (enemy->collidesWithItem(tile)) {
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
            MinionEnemy* minion = dynamic_cast<MinionEnemy*>(enemy);
            bool movingRight = minion ? minion->isFacingRight() : (enemy->vx > 0);
            qreal lookAheadX = movingRight ? eRect.right() + 7 : eRect.left() - 7;
            qreal footY = eRect.bottom() + 5; // 往脚底下偏离2个像素，确保能踩到地面
            QPointF checkPoint(lookAheadX, footY);

            bool hasFloorAhead = false;
            for (Tile *tile : floors) {
                // 检查这个探测点是否在某个地面方块的矩形内
                if (tile->sceneBoundingRect().contains(checkPoint)) {
                    hasFloorAhead = true;
                    break; // 前方有地基，安全
                }
            }

            // 如果前方没有地面，说明到边缘了，掉头
            if (!hasFloorAhead) {
                enemy->reverseDirection();
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

    // ====== Boss动态生成（战斗关卡：玩家靠近末尾时创建Boss） ======
    if (pendingBossType > 0 && !bossSpawned && player && player->x() > bossSpawnX - 300) {
        if (pendingBossType == 1) {
            brainOfCthulhu = new BrainOfCthulhu(player);
            brainOfCthulhu->setPos(bossSpawnX, bossSpawnY);
            brainOfCthulhu->setVisible(false);
            scene->addItem(brainOfCthulhu);
            enemies.append(brainOfCthulhu);
        } else if (pendingBossType == 2) {
            dukeFishron = new DukeFishron(player);
            dukeFishron->setPos(bossSpawnX, bossSpawnY);
            dukeFishron->setVisible(false);
            scene->addItem(dukeFishron);
            enemies.append(dukeFishron);
        }
        bossSpawned = true;
    }

    // ====== Boss接近激活 ======
    const qreal bossActDist = 600;
    if (brainOfCthulhu && !brainOfCthulhu->isDead && !brainOfCthulhu->isVisible()) {
        if (qAbs(player->x() - brainOfCthulhu->x()) < bossActDist)
            brainOfCthulhu->setVisible(true);
    }
    if (dukeFishron && !dukeFishron->isDead && !dukeFishron->isVisible()) {
        if (qAbs(player->x() - dukeFishron->x()) < bossActDist)
            dukeFishron->setVisible(true);
    }
    if (iceGod && !iceGod->isDead && !iceGod->isVisible()) {
        if (qAbs(player->x() - iceGod->x()) < bossActDist)
            iceGod->setVisible(true);
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
            for (Tile *tile : waters) {
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
        QRectF pRect = player->sceneBoundingRect();

        for (Tile* spike : spikes) {
            QRectF sRect = spike->sceneBoundingRect();
            // 缩小判定范围为刺的实际视觉区域（内缩60%）
            qreal sx = sRect.width() * 0.35;
            qreal sy = sRect.height() * 0.35;
            sRect.adjust(sx, sy, -sx, -sy);

            if (pRect.intersects(sRect)) {

                // 技能免伤保护
                if (player->isRolling || player->isSwallowing ||
                    player->isIceDefending || player->isExploding) {
                    continue;
                }

                // 触发扣血
                player->hp--;
                player->invulnTimer = 60;
                player->vy = -6; // 受击向上弹

                // 根据卡比朝向，精准反向弹飞，防止卡在刺里连续扣血
                if (player->facingRight) {
                    player->setPos(player->x() - 15, player->y());
                } else {
                    player->setPos(player->x() + 15, player->y());
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
            timer->stop();

            QGraphicsTextItem* winText = new QGraphicsTextItem("MISSION ACCOMPLISHED!");
            winText->setFont(QFont("SimHei", 36, QFont::Bold));
            winText->setDefaultTextColor(QColor(255, 215, 0));
            winText->setZValue(2000);
            qreal tw = winText->boundingRect().width();
            winText->setPos(player->x() - tw / 2.0, 400);
            scene->addItem(winText);

            QTimer::singleShot(2500, this, [this, winText]() {
                scene->removeItem(winText);
                delete winText;
                cleanupGameObjects();
                showMainMenu();
            });
            return;
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

void MainWindow::mousePressEvent(QMouseEvent *event) { Q_UNUSED(event); }
void MainWindow::mouseMoveEvent(QMouseEvent *event) { Q_UNUSED(event); }

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
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
                // 返回按钮点击
                if (event->type() == QEvent::MouseButtonPress &&
                    static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                    if (!settingsBtnRects.isEmpty() && settingsBtnRects[0].contains(scenePos)) {
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

            // === 选关界面悬停/点击 ===
            if (currentState == LevelSelect) {
                for (int i = 0; i < cardRects.size() && i < levelCards.size(); i++) {
                    bool inside = cardRects[i].contains(scenePos);
                    bool isContinueCard = (i < cardLevelNums.size() && cardLevelNums[i] == -1);
                    QColor normal = isContinueCard ? QColor(30, 70, 30, 210) : QColor(30, 30, 70, 210);
                    QColor hover = isContinueCard ? QColor(50, 140, 50, 230) : QColor(60, 60, 140, 230);
                    QPen normalPen = isContinueCard ? QPen(QColor(100, 200, 100), 2) : QPen(QColor(80, 80, 180), 2);
                    if (levelCards[i]) {
                        levelCards[i]->setBrush(inside ? QBrush(hover) : QBrush(normal));
                        levelCards[i]->setPen(inside ? QPen(QColor(200, 200, 100), 3) : normalPen);
                    }
                    if (inside && event->type() == QEvent::MouseButtonPress &&
                        static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                        if (isContinueCard) {
                            pendingAction = ACT_CONTINUE_GAME;
                            return true;
                        }
                        int lvNum = (i < cardLevelNums.size()) ? cardLevelNums[i] : 1;
                        keys.insert(Qt::Key_1 + lvNum - 1);
                        return true;
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
                if (event->type() == QEvent::MouseButtonPress &&
                    static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
                    for (int i = 0; i < pauseBtnRects.size(); i++) {
                        if (pauseBtnRects[i].contains(scenePos)) {
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

void MainWindow::cleanupGameObjects() {
    for (Enemy* t : enemies) { scene->removeItem(t); delete t; }
    enemies.clear();
    for (Tile* t : waters) { scene->removeItem(t); delete t; }
    waters.clear();
    for (Tile* t : floors) { scene->removeItem(t); delete t; }
    floors.clear();
    spikes.clear();
    for (Cake* c : cakes) { scene->removeItem(c); delete c; }
    cakes.clear();
    for (Checkpoint* cp : checkpoints) { scene->removeItem(cp); delete cp; }
    checkpoints.clear();
    for (Goal* g : goals) { scene->removeItem(g); delete g; }
    goals.clear();
    for (auto* ex : exhaustItems) { scene->removeItem(ex); delete ex; }
    exhaustItems.clear(); exhaustLifetimes.clear();
    // Boss已在enemies循环中delete，置空防止悬空指针
    dukeFishron = nullptr;
    brainOfCthulhu = nullptr;
    iceGod = nullptr;
    if (player) { scene->removeItem(player); delete player; player = nullptr; }
    staminaBar->setVisible(false);
    for (auto icon : lifeIcons) icon->setVisible(false);
    if (cooldownText) cooldownText->setVisible(false);
    if (bossHpBarBg) bossHpBarBg->setVisible(false);
    if (bossHpBarFg) bossHpBarFg->setVisible(false);
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
    qreal vw = view->viewport()->width();
    qreal vh = view->viewport()->height();
    qreal cx = camX - vw / 2.0;
    qreal cy = camY - vh / 2.0;

    gameOverOverlay = new QGraphicsRectItem(cx, cy, vw, vh);
    gameOverOverlay->setBrush(QBrush(QColor(0, 0, 0, 180)));
    gameOverOverlay->setPen(Qt::NoPen);
    gameOverOverlay->setZValue(1999);
    scene->addItem(gameOverOverlay);

    gameOverTitle = new QGraphicsTextItem("GAME OVER");
    gameOverTitle->setFont(QFont("SimHei", 48, QFont::Bold));
    gameOverTitle->setDefaultTextColor(QColor(255, 80, 80));
    gameOverTitle->setZValue(2000);
    double tw = gameOverTitle->boundingRect().width();
    gameOverTitle->setPos(cx + (vw - tw) / 2.0, cy + 80);
    scene->addItem(gameOverTitle);

    qreal btnW = 260, btnH = 50, btnSpacing = 20;
    qreal btnX = cx + (vw - btnW) / 2.0;
    qreal btnY = cy + vh / 2.0 + 20;

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

void MainWindow::showStartScreen() {
    if (!titleText) {
        titleText = new QGraphicsTextItem("星之卡比");
        titleText->setFont(QFont("SimHei", 48, QFont::Bold));
        titleText->setDefaultTextColor(Qt::white);
        titleText->setZValue(2000);
        titleText->setPos(380, 650);
        scene->addItem(titleText);
    } else { titleText->setVisible(true); }
    if (!hintText) {
        hintText = new QGraphicsTextItem("- 按ENTER开始游戏 -");
        hintText->setFont(QFont("SimHei", 20, QFont::Bold));
        hintText->setDefaultTextColor(Qt::yellow);
        hintText->setZValue(2000);
        hintText->setPos(380, 780);
        scene->addItem(hintText);
    } else { hintText->setVisible(true); }
    if (!player) {
        player = new Player();
        player->setPos(800, 856);
        scene->addItem(player);
    }
    currentState = START_SCREEN;
    timer->start(16);
    enterPressed = false;
}

void MainWindow::cleanupMainMenuUI() {
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
    cleanupMainMenuUI();
    view->centerOn(500, 900);
    qreal vw = view->viewport()->width();
    qreal vh = view->viewport()->height();
    qreal cx = 500 - vw / 2.0;
    qreal cy = 900 - vh / 2.0;

    mainMenuOverlay = new QGraphicsRectItem(cx, cy, vw, vh);
    mainMenuOverlay->setBrush(QBrush(QColor(0, 0, 0, 180)));
    mainMenuOverlay->setPen(Qt::NoPen);
    mainMenuOverlay->setZValue(1999);
    scene->addItem(mainMenuOverlay);

    mainMenuTitle = new QGraphicsTextItem("星之卡比");
    mainMenuTitle->setFont(QFont("SimHei", 42, QFont::Bold));
    mainMenuTitle->setDefaultTextColor(QColor(255, 255, 100));
    mainMenuTitle->setZValue(2000);
    mainMenuTitle->setPos(cx + (vw - mainMenuTitle->boundingRect().width()) / 2.0, cy + 40);
    scene->addItem(mainMenuTitle);

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
    qreal btnX = cx + (vw - btnW) / 2.0;
    qreal btnY = cy + 130;
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

    view->centerOn(500, 900);
    for (QGraphicsRectItem* bg : backgroundLayers) {
        bg->setPos(500 - bg->rect().width() / 2.0, 0);
    }
    currentState = MAIN_MENU;
    timer->start(16);
}

void MainWindow::showSettings() {
    cleanupSettingsUI();
    view->centerOn(500, 900);
    qreal vw = view->viewport()->width();
    qreal vh = view->viewport()->height();
    qreal cx = 500 - vw / 2.0;
    qreal cy = 900 - vh / 2.0;

    settingsOverlay = new QGraphicsRectItem(cx, cy, vw, vh);
    settingsOverlay->setBrush(QBrush(QColor(0, 0, 0, 180)));
    settingsOverlay->setPen(Qt::NoPen);
    settingsOverlay->setZValue(1999);
    scene->addItem(settingsOverlay);

    settingsTitle = new QGraphicsTextItem("设置");
    settingsTitle->setFont(QFont("SimHei", 36, QFont::Bold));
    settingsTitle->setDefaultTextColor(QColor(255, 255, 100));
    settingsTitle->setZValue(2000);
    settingsTitle->setPos(cx + (vw - settingsTitle->boundingRect().width()) / 2.0, cy + 40);
    scene->addItem(settingsTitle);

    qreal btnW = 260, btnH = 50;
    qreal btnX = cx + (vw - btnW) / 2.0;
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

    view->centerOn(500, 900);
    for (QGraphicsRectItem* bg : backgroundLayers) {
        bg->setPos(500 - bg->rect().width() / 2.0, 0);
    }
    currentState = SETTINGS;
}

void MainWindow::showLevelSelect() {
    cleanupSelectUI();
    view->centerOn(500, 900);
    qreal vw = view->viewport()->width();
    qreal vh = view->viewport()->height();
    qreal cx = 500 - vw / 2.0;
    qreal cy = 900 - vh / 2.0;

    selectOverlay = new QGraphicsRectItem(cx, cy, vw, vh);
    selectOverlay->setBrush(QBrush(QColor(0, 0, 0, 180)));
    selectOverlay->setPen(Qt::NoPen);
    selectOverlay->setZValue(1999);
    scene->addItem(selectOverlay);

    selectTitle = new QGraphicsTextItem("选择关卡");
    selectTitle->setFont(QFont("SimHei", 36, QFont::Bold));
    selectTitle->setDefaultTextColor(QColor(255, 255, 100));
    selectTitle->setZValue(2000);
    selectTitle->setPos(cx + (vw - selectTitle->boundingRect().width()) / 2.0, cy + 20);
    scene->addItem(selectTitle);

    // ====== 三栏分类布局 ======
    struct LevelInfo { int num; QString name; QString desc; QColor bg; QColor border; };
    struct Category { QString name; QString icon; QColor headerColor; QList<LevelInfo> levels; };

    QList<Category> cats;
    cats.append({"战斗关卡", "⚔", QColor(255, 120, 80), {
        {1, "星之绿地", "Boss: 克苏鲁之脑", QColor(70, 30, 30, 210), QColor(200, 80, 80)},
        {5, "熔岩堡垒", "Boss: 猪鲨公爵",    QColor(70, 40, 20, 210), QColor(220, 130, 50)},
    }});
    cats.append({"跑酷关卡", "🏃", QColor(80, 160, 255), {
        {2, "刺之回廊", "移动刺 · 纯跳跃",   QColor(30, 40, 80, 210), QColor(80, 120, 220)},
        {4, "云中回廊", "浮空平台 · 连续翻滚", QColor(30, 50, 90, 210), QColor(100, 150, 240)},
    }});
    cats.append({"坑爹关卡", "💀", QColor(80, 220, 100), {
        {3, "寒冰炼狱", "苹果 · 伏击刺 · 冰砖", QColor(30, 60, 40, 210), QColor(80, 200, 100)},
        {6, "暗影洞穴", "双重陷阱 · 上下交错", QColor(30, 50, 40, 210), QColor(100, 200, 120)},
    }});

    cardRects.clear(); cardLevelNums.clear();
    qreal colW = vw / 3.0;
    qreal cardW = 160, cardH = 60;
    qreal cardSpacing = 10;
    qreal colStartY = cy + 80;

    for (int ci = 0; ci < cats.size(); ci++) {
        qreal colX = cx + ci * colW;
        qreal curY = colStartY;

        // 分类标题
        QGraphicsTextItem* catHeader = new QGraphicsTextItem(cats[ci].icon + " " + cats[ci].name);
        catHeader->setFont(QFont("SimHei", 16, QFont::Bold));
        catHeader->setDefaultTextColor(cats[ci].headerColor);
        catHeader->setZValue(2000);
        catHeader->setPos(colX + (colW - cardW) / 2.0 + 10, curY);
        scene->addItem(catHeader);
        categoryHeaders.append(catHeader);
        curY += 30;

        // 该分类下的关卡卡片
        for (auto& lv : cats[ci].levels) {
            qreal cardX = colX + (colW - cardW) / 2.0;
            QRectF cr(cardX, curY, cardW, cardH);
            cardRects.append(cr);
            cardLevelNums.append(lv.num);

            QGraphicsRectItem* card = new QGraphicsRectItem(cr);
            card->setBrush(QBrush(lv.bg));
            card->setPen(QPen(lv.border, 2));
            card->setZValue(2000);
            scene->addItem(card);
            levelCards.append(card);

            QGraphicsTextItem* label = new QGraphicsTextItem(QString("关卡%1\n%2").arg(lv.num).arg(lv.name));
            label->setFont(QFont("SimHei", 11, QFont::Bold));
            label->setDefaultTextColor(Qt::white);
            label->setZValue(2001);
            double tw = label->boundingRect().width();
            double th = label->boundingRect().height();
            label->setPos(cardX + (cardW - tw) / 2.0, curY + (cardH - th) / 2.0);
            scene->addItem(label);
            cardLabels.append(label);

            curY += cardH + cardSpacing;
        }

        // 继续游戏卡片（如果有存档且是该分类的关卡）
        if (savedLevelNum > 0 && ci == 0) {
            // 在战斗栏底部放继续游戏卡片
            qreal cardX = colX + (colW - cardW) / 2.0;
            QRectF cr(cardX, curY + 15, cardW, cardH);
            cardRects.append(cr);
            cardLevelNums.append(-1); // -1 表示继续游戏

            QGraphicsRectItem* card = new QGraphicsRectItem(cr);
            card->setBrush(QBrush(QColor(30, 70, 30, 210)));
            card->setPen(QPen(QColor(100, 200, 100), 2));
            card->setZValue(2000);
            scene->addItem(card);
            levelCards.append(card);

            QGraphicsTextItem* label = new QGraphicsTextItem(QString("继续游戏\n(关卡%1)").arg(savedLevelNum));
            label->setFont(QFont("SimHei", 11, QFont::Bold));
            label->setDefaultTextColor(QColor(180, 255, 180));
            label->setZValue(2001);
            double tw = label->boundingRect().width();
            double th = label->boundingRect().height();
            label->setPos(cardX + (cardW - tw) / 2.0, cr.y() + (cardH - th) / 2.0);
            scene->addItem(label);
            cardLabels.append(label);
        }
    }

    // 底部提示
    menuText = new QGraphicsTextItem("- 按数字键1-6选关 · 按ESC返回主菜单 -");
    menuText->setFont(QFont("SimHei", 13, QFont::Bold));
    menuText->setDefaultTextColor(QColor(200, 200, 200));
    menuText->setZValue(2000);
    double tw = menuText->boundingRect().width();
    menuText->setPos(cx + (vw - tw) / 2.0, colStartY + 210);
    scene->addItem(menuText);

    view->centerOn(500, 900);
    for (QGraphicsRectItem* bg : backgroundLayers) {
        bg->setPos(500 - bg->rect().width() / 2.0, 0);
    }
    currentState = LevelSelect;
    timer->start(16);
}