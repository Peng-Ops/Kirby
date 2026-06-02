#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QTimer>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSet>
#include <QGraphicsTextItem>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QProgressBar>
#include "player.h"
#include "tile.h"
#include "enemy.h"
#include "projectile.h"
#include "cake.h"
#include "checkpoint.h"
#include "goal.h"
#include "dukefishron.h"
#include "brainofcthulhu.h"
#include "icegod.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void gameUpdate();

private:
    QList<QGraphicsPixmapItem*> lifeIcons; // 存放生命值图标的列表
    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QGraphicsView *view;
    QTimer *timer;
    Player *player;
    QPixmap grass, dirt, waterSurface, waterBodyPix, iceBlockPix, rubblePix, ciPix, ci2Pix, ci3Pix, qiangciPix, daociPix;
    QList<QGraphicsRectItem*> backgroundLayers; // 新增：用于记录背景图层
    int lastHorizontalKey = 0;
    QList<Tile*> floors;
    QList<Tile*> waters;
    QList<Tile*> spikes;  // 刺方块（对玩家造成伤害）
    QSet<int> keys;
    int jumpBuffer = 0;
    int coyoteTime = 0;
    QList<Enemy*> enemies;
    DukeFishron* dukeFishron = nullptr;        // 猪鲨Boss
    BrainOfCthulhu* brainOfCthulhu = nullptr;  // 克苏鲁之脑Boss
    IceGod* iceGod = nullptr;                  // 冰雪之神Boss
    QGraphicsRectItem* bossHpBarBg = nullptr;  // Boss血条背景
    QGraphicsRectItem* bossHpBarFg = nullptr;  // Boss血条前景
    QGraphicsTextItem* cooldownText = nullptr;   // 技能冷却HUD（右下角）
    QList<Projectile*> projectiles;
    QList<Cake*> cakes;
    QList<Checkpoint*> checkpoints;
    QList<Goal*> goals;
    QPointF lastCheckpointPos;  // 最新激活的检查点复活位置
    bool hasCheckpoint = false; // 是否已激活过检查点
    QVector<QPixmap> weiqiFrames;
    QList<QGraphicsPixmapItem*> exhaustItems;
    QList<int> exhaustLifetimes;
    QMediaPlayer *bgmPlayer;
    QAudioOutput *audioOutput;
    int aiTimer = 0;
    bool enterPressed = false;
    QProgressBar* staminaBar; // 新增体力条指针

    // ====== 新增：游戏状态机枚举与控制变量 ======
    enum GameState { START_SCREEN, LevelSelect, INTRO_PAN, PLAYING };
    GameState currentState = START_SCREEN;
    int introTimer = 0;
    void loadLevel(int levelNum); // 把你之前那一长串解析地图的代码，封装进这个函数
    QGraphicsTextItem* menuText;  // 用于在屏幕上显示提示文字

    // UI文字元素
    QGraphicsTextItem* titleText = nullptr;
    QGraphicsTextItem* hintText = nullptr;

    // ====== 主菜单UI ======
    QGraphicsRectItem* mainMenuOverlay = nullptr;
    QGraphicsTextItem* mainMenuTitle = nullptr;
    QList<QGraphicsRectItem*> mainMenuButtons;
    QList<QGraphicsTextItem*> mainMenuBtnTexts;
    QList<QRectF> mainMenuBtnRects;
    int mainMenuBtnCount = 0;  // 实际按钮数（可能3或4，取决于是否有存档）

    // ====== 设置界面UI ======
    QGraphicsRectItem* settingsOverlay = nullptr;
    QGraphicsTextItem* settingsTitle = nullptr;
    QList<QGraphicsRectItem*> settingsButtons;
    QList<QGraphicsTextItem*> settingsBtnTexts;
    QList<QRectF> settingsBtnRects;
    QGraphicsRectItem* settingsVolBg = nullptr;
    QGraphicsRectItem* settingsVolFg = nullptr;
    QGraphicsRectItem* settingsVolHandle = nullptr;
    QRectF settingsVolRect;
    bool settingsDragging = false;

    // ====== 选关UI ======
    QGraphicsRectItem* selectOverlay = nullptr;
    QGraphicsTextItem* selectTitle = nullptr;
    QList<QGraphicsRectItem*> levelCards;
    QList<QGraphicsTextItem*> cardLabels;
    QList<QRectF> cardRects;
    QList<int> cardLevelNums;  // 每个卡片对应的关卡号
    QList<QGraphicsTextItem*> categoryHeaders;

    // ====== 暂停菜单 ======
    QGraphicsRectItem* pauseOverlay = nullptr;
    QGraphicsTextItem* pauseTitle = nullptr;
    QList<QGraphicsRectItem*> pauseButtons;
    QList<QGraphicsTextItem*> pauseButtonTexts;
    QList<QRectF> pauseBtnRects;
    QGraphicsRectItem* volTrackBg = nullptr;
    QGraphicsRectItem* volTrackFg = nullptr;
    QGraphicsRectItem* volHandle = nullptr;
    QRectF volTrackRect;
    bool isDraggingVolume = false;
    double volumeLevel = 0.5;

    // ====== 游戏结束UI ======
    QGraphicsRectItem* gameOverOverlay = nullptr;
    QGraphicsTextItem* gameOverTitle = nullptr;
    QList<QGraphicsRectItem*> gameOverButtons;
    QList<QGraphicsTextItem*> gameOverButtonTexts;
    QList<QRectF> gameOverBtnRects;

    // ====== 存档状态 ======
    int currentLevelNum = 0;
    int savedLevelNum = 0;
    QPointF savedCheckpointPos;
    Enemy::CopyAbility savedForm = Enemy::NONE;
    int savedHP = 3;
    int savedStamina = 300;
    int savedAttackPowerTimer = 0;

    // ====== Boss动态生成 ======
    int pendingBossType = 0;    // 0=无, 1=克苏鲁, 2=猪鲨, 3=冰雪
    qreal bossSpawnX = 0;
    qreal bossSpawnY = 400;
    bool bossSpawned = false;

    // 辅助方法
    void cleanupPauseUI();
    void cleanupGameObjects();
    void cleanupSelectUI();
    void cleanupMainMenuUI();
    void cleanupSettingsUI();
    void showMainMenu();
    void showSettings();
    void showStartScreen();
    void showLevelSelect();
    void showGameOver();
    void cleanupGameOverUI();

};

#endif // MAINWINDOW_H