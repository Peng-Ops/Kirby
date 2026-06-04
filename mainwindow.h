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
#include "crate.h"
#include "dukefishron.h"
#include "brainofcthulhu.h"
#include "icegod.h"
#include "star.h"
#include "badge.h"
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 屏幕尺寸常量（窗口固定为 1000x700）
constexpr qreal SCREEN_W = 1000.0;
constexpr qreal SCREEN_H = 700.0;

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
    int playerMaxHp = 5;
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
    QGraphicsPixmapItem* bossHpBarFrame = nullptr;  // Boss血条外框 (Outer_Lower)
    QGraphicsRectItem* bossHpBarFill = nullptr;      // Boss血条红色填充
    QPixmap bossHpBarFramePixmap;                    // Outer_Lower贴图缓存
    QGraphicsTextItem* cooldownText = nullptr;   // 技能冷却HUD（右下角）
    QGraphicsPixmapItem* fireCdIcon = nullptr;    // 火形态冷却图标
    QGraphicsPixmapItem* iceCdIcon = nullptr;     // 冰形态冷却图标
    QGraphicsPixmapItem* leafCdIcon = nullptr;    // 叶形态冷却图标
    QList<Projectile*> projectiles;
    QList<Cake*> cakes;
    QList<Star*> stars;         // Boss战星星道具
    int bossStarSpawnTimer = 0; // Boss战星星生成计时器
    QList<Checkpoint*> checkpoints;
    QList<Goal*> goals;
    QList<Crate*> crates;           // 木箱列表
    QPointF lastCheckpointPos;  // 最新激活的检查点复活位置
    bool hasCheckpoint = false; // 是否已激活过检查点
    QVector<QPixmap> weiqiFrames;
    QList<QGraphicsPixmapItem*> exhaustItems;
    QList<int> exhaustLifetimes;
    QMediaPlayer *bgmPlayer;
    QAudioOutput *audioOutput;
    QString originalBgmSource;       // 保存原始BGM，退出Boss战时恢复
    // 音效系统
    QMediaPlayer *stepPlayer;       // 脚步循环
    QAudioOutput *stepAudio;
    QMediaPlayer *waterPlayer;      // 水声循环
    QAudioOutput *waterAudio;
    QMediaPlayer *sfxPlayer;        // 短音效 (pop, 扣血)
    QAudioOutput *sfxAudio;
    QMediaPlayer *cinematicPlayer;  // 剧情音效 (死亡, 过关)
    QAudioOutput *cinematicAudio;
    bool bgmPaused = false;
    bool prevExploding = false;     // 爆炸音效边缘检测
    bool cratePushSoundPlayed = false; // 木箱推动音效每帧只播一次
    int aiTimer = 0;
    int screenShakeTimer = 0;
    qreal actualCameraX = 500.0;    // 当前实际相机X（已做边界钳制）
    qreal actualCameraY = 350.0;    // 当前实际相机Y
    QProgressBar* staminaBar; // 新增体力条指针

    // ====== DukeFishron二阶段背景过渡 ======
    QList<QGraphicsRectItem*> phase2BgLayers;
    bool dukePhase2BgStarted = false;
    int phase2BgFadeTimer = 0;

    // ====== 浮动文字教程系统 ======
    struct TutorialTrigger {
        qreal triggerX;
        QString message;
        int durationFrames = 240;
        bool shown = false;
        QGraphicsTextItem* item = nullptr;
        int remainingFrames = 0;
    };
    QVector<TutorialTrigger> tutorialTriggers;
    void triggerTutorialText(TutorialTrigger* t);
    void updateTutorialTexts();
    void cleanupTutorialTexts();


    // ====== 游戏状态机枚举与控制变量 ======
    enum GameState { START_SCREEN, LevelSelect, INTRO_PAN, PLAYING, PAUSED, SETTINGS, MAIN_MENU, GAME_OVER, ENDING };
    GameState currentState = MAIN_MENU;

    // ====== 待处理动作枚举（用于 eventFilter 与 gameUpdate 之间的桥接） ======
    enum PendingAction {
        ACT_NONE,
        ACT_SHOW_LEVEL_SELECT,
        ACT_CONTINUE_GAME,
        ACT_SHOW_SETTINGS,
        ACT_SHOW_MAIN_MENU,
        ACT_RESUME_GAME,
        ACT_RESTART_CHECKPOINT,
        ACT_EXIT
    };
    PendingAction pendingAction = ACT_NONE;
    void loadLevel(int levelNum); // 把你之前那一长串解析地图的代码，封装进这个函数
    QGraphicsTextItem* menuText = nullptr;  // 用于在屏幕上显示提示文字

    // ====== 主菜单UI ======
    QGraphicsRectItem* mainMenuOverlay = nullptr;
    QGraphicsPixmapItem* mainMenuBg0 = nullptr;  // kaishidonghua 背景帧0
    QGraphicsPixmapItem* mainMenuBg1 = nullptr;  // kaishidonghua 背景帧1
    QGraphicsTextItem* mainMenuTitle = nullptr;
    QList<QGraphicsRectItem*> mainMenuButtons;
    QList<QGraphicsPixmapItem*> mainMenuBtnImages;
    QList<QGraphicsTextItem*> mainMenuBtnTexts;
    QList<QRectF> mainMenuBtnRects;
    int mainMenuBtnCount = 0;  // 实际按钮数（可能3或4，取决于是否有存档）

    // ====== 设置界面UI ======
    QGraphicsPixmapItem* settingsPanel = nullptr;    // setting.png
    QGraphicsPixmapItem* settingsBackBtn = nullptr;  // restart返回按钮
    QRectF settingsBackRect;
    QList<QGraphicsRectItem*> settingsButtons;       // 辅助清理
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
    int modeSelection = 0;     // 模式选择标记 (0=无, 1=冒险, 21=Boss1克苏鲁, 22=Boss2猪鲨, 23=Boss3冰雪)

    // ====== Boss选择子菜单 ======
    bool isBossSelect = false;
    QVector<QGraphicsRectItem*> bossCards;
    QVector<QGraphicsTextItem*> bossCardLabels;
    QVector<QGraphicsPixmapItem*> bossCardImages;
    QVector<QRectF> bossCardRects;
    QVector<int> bossSelectTypes;   // 1,2,3 对应三种Boss
    QGraphicsTextItem* bossBackLabel = nullptr;
    void showBossSelect();

    // ====== 暂停菜单 ======
    QGraphicsRectItem* pauseOverlay = nullptr;
    QGraphicsTextItem* pauseTitle = nullptr;
    QGraphicsPixmapItem* pausePanelBg = nullptr;  // 暂停面板背景(setting.png)
    QList<QGraphicsRectItem*> pauseButtons;
    QList<QGraphicsTextItem*> pauseButtonTexts;
    QList<QGraphicsPixmapItem*> pauseBtnImages;
    QList<QRectF> pauseBtnRects;
    QGraphicsRectItem* volTrackBg = nullptr;
    QGraphicsRectItem* volTrackFg = nullptr;
    QGraphicsRectItem* volHandle = nullptr;
    QRectF volTrackRect;
    bool isDraggingVolume = false;
    double volumeLevel = 0.5;

    // ====== 游戏结束UI ======
    QGraphicsPixmapItem* gameOverPanel = nullptr;   // 面板背景
    QList<QGraphicsPixmapItem*> gameOverBtnImages;
    QList<QRectF> gameOverBtnRects;

    // ====== 存档状态 ======
    int currentLevelNum = 0;
    int savedLevelNum = 0;
    QPointF savedCheckpointPos;
    Enemy::CopyAbility savedForm = Enemy::NONE;
    QList<Enemy::CopyAbility> savedAbilities;
    int savedHP = 3;
    int savedStamina = 300;
    int savedAttackPowerTimer = 0;

    // ====== 360度瞄准系统（Boss模式专用）======
    double shootAngle = 0.0;

    // ====== Boss动态生成 ======
    int pendingBossType = 0;    // 0=无, 1=克苏鲁, 2=猪鲨, 3=冰雪
    qreal bossSpawnX = 0;
    qreal bossSpawnY = 400;
    bool bossSpawned = false;

    // ====== Boss战小怪生成 ======
    int bossMinionSpawnTimer = 0;   // 每帧+1，满1800(30秒)生成一波
    // Boss死亡白屏+徽章
    QGraphicsRectItem* whiteOverlay = nullptr;  // 白屏遮罩
    Badge* droppedBadge = nullptr;              // Boss掉落的徽章
    int bossDeathGlobalPhase = 0;               // 0=无 1=闪烁 2=发光 3=白屏 4=等待拾取
    int bossDeathGlobalTimer = 0;               // 死亡全局计时器
    int dyingBossType = 0;                      // 1=克苏鲁之脑 2=猪鲨 3=冰雪女王
    qreal dyingBossX = 0, dyingBossY = 0;       // Boss死亡位置(掉落徽章位置)

    // ====== Boss登场动画 ======
    int bossIntroTimer = 0;         // 帧计数器，0=未开始
    bool bossIntroActive = false;
    QGraphicsRectItem* bossIntroOverlay = nullptr;    // 红色遮罩
    QGraphicsTextItem* bossIntroExclamation = nullptr; // 白色感叹号
    QMediaPlayer* bossStingerPlayer;                  // horror音效
    QAudioOutput* bossStingerAudio;
    QStringList bossMusicTracks;                      // RPG战斗音乐列表

    // 辅助方法
    void cleanupPauseUI();
    void cleanupGameObjects();
    void cleanupSelectUI();
    void cleanupMainMenuUI();
    void cleanupSettingsUI();
    void cleanupEndingUI();
    void showMainMenu();
    void showSettings();
    void showLevelSelect();
    void showGameOver();
    void cleanupGameOverUI();
    // 结束动画
    int endingAnimTimer = 0;
    int endingStage = 0;
    QGraphicsPixmapItem* endingBg = nullptr;
    QGraphicsPixmapItem* endingRock = nullptr;
    QGraphicsPixmapItem* endingKirby = nullptr;
    QGraphicsPixmapItem* endingBigBird = nullptr;
    QGraphicsPixmapItem* endingSmallBird = nullptr;
    QGraphicsPixmapItem* endingScreen = nullptr;
    QGraphicsRectItem* endingOverlay = nullptr;
    QGraphicsRectItem* endingBlackBg = nullptr;  // 结束动画黑底（需清理防止泄漏）
    QGraphicsPixmapItem* endingMenuBtn = nullptr;   // restart图标
    QRectF endingMenuBtnRect;                        // 点击区域
    QVector<QPixmap> endingKirbyFrames;
    QVector<QPixmap> endingBigBirdFrames;
    QVector<QPixmap> endingSmallBirdFrames;
    qreal endingBaseX = 0, endingBaseY = 0;
    void startEndingAnimation();

};

#endif // MAINWINDOW_H