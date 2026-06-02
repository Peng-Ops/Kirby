#ifndef APPLE_H
#define APPLE_H

#include "enemy.h"
#include <QGraphicsTextItem>

class Apple : public Enemy {
public:
    Apple();

    void updateLogic() override;
    bool canBeSwallowed() override { return false; } // 苹果不能被吞噬

    enum State { IDLE, WARNING, GROWING, SHRINKING };
    State state = IDLE;

private:
    int warningTimer = 0;      // 预警60帧
    int growTimer = 0;         // 变大120帧
    int shrinkTimer = 0;       // 缩小60帧
    int pauseTimer = 0;        // 最大后停顿60帧
    qreal baseScale = 1.0;
    qreal maxScale = 2.5;
    QGraphicsTextItem* warningIndicator = nullptr;
};

#endif // APPLE_H
