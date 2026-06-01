#ifndef GOAL_H
#define GOAL_H

#include <QGraphicsPixmapItem>

class Goal : public QGraphicsPixmapItem {
public:
    Goal();
    bool isReached = false;
};

#endif // GOAL_H
