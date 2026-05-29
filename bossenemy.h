#ifndef BOSSENEMY_H
#define BOSSENEMY_H

#include "enemy.h"

// Boss类：Boss敌人基类，无视地形碰撞，不可被吞噬
class BossEnemy : public Enemy {
public:
    BossEnemy() {
        ignoresTiles = true;   // Boss可穿墙飞行，无视地形
    }

    bool canBeSwallowed() override { return false; }  // Boss不可被吞噬
};

#endif // BOSSENEMY_H
