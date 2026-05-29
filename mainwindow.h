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
#include "player.h"
#include "tile.h"
#include "enemy.h"
#include "projectile.h"
#include "cake.h"
#include "checkpoint.h"
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

private slots:
    void gameUpdate();

private:
    QList<QGraphicsPixmapItem*> lifeIcons; // 存放生命值图标的列表
    void loadLevel(int level);
    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QGraphicsView *view;
    QTimer *timer;
    Player *player;
    QPixmap waterBodyPix; // 新增：保存水的贴图以备融冰之用
    QList<QGraphicsRectItem*> backgroundLayers; // 新增：用于记录背景图层
    int lastHorizontalKey = 0;
    QList<Tile*> floors;
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
    QPointF lastCheckpointPos;  // 最新激活的检查点复活位置
    bool hasCheckpoint = false; // 是否已激活过检查点
    QVector<QPixmap> weiqiFrames;
    QList<QGraphicsPixmapItem*> exhaustItems;
    QList<int> exhaustLifetimes;
    QMediaPlayer *bgmPlayer;
    QAudioOutput *audioOutput;
    int aiTimer = 0;
    bool enterPressed = false;

    // ====== 新增：游戏状态机枚举与控制变量 ======
    enum GameState { START_SCREEN, INTRO_PAN, PLAYING };
    GameState currentState = START_SCREEN;
    int introTimer = 0;

    // UI文字元素
    QGraphicsTextItem* titleText = nullptr;
    QGraphicsTextItem* hintText = nullptr;

};

#endif // MAINWINDOW_H