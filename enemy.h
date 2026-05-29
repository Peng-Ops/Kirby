#ifndef ENEMY_H
#define ENEMY_H

#include "gameobject.h"

class Enemy : public GameObject {
public:
    // 定义一个枚举，列出游戏中所有的复制能力
    enum CopyAbility {
        NONE,       // 无能力（比如普通的瓦豆鲁迪）
        FIRE,       // 火能力
        WATER,      // 水能力
        ICE,        // 冰能力
        SPARK,      // 电能力
        LEAF        // 叶能力
    };
    virtual void reverseDirection() {}
    Enemy();

    int hp = 1;
    bool isDead = false;
    int damage = 1;

    // 给每个敌人身上带一个能力标签，默认没有能力
    CopyAbility ability = NONE;

    // Boss相关标记：是否无视地形碰撞（可穿墙飞行）
    bool ignoresTiles = false;
    // Boss相关标记：是否可被卡比吞噬
    virtual bool canBeSwallowed() { return true; }

    virtual void takeDamage(int dmg) {
        hp -= dmg;
        if (hp <= 0) {
            isDead = true;
        }
    }
};

#endif // ENEMY_H