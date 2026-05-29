#ifndef TILE_H
#define TILE_H

#include "gameobject.h"

class Tile : public GameObject {
public:
    enum TileType { Grass, Dirt, WaterSurface, WaterBody, IceBlock, RubbleBlock, Spike };

    Tile(TileType type, const QPixmap &pixmap);

    TileType tileType() const { return m_type; }
    void updateLogic() override;

    void changeType(TileType newType, const QPixmap &newPix);

    // 运动参数（ci/ci2/ci3 等刺方块可用）
    bool canMove = false;
    enum MoveAxis { None, Horizontal, Vertical };
    MoveAxis moveAxis = None;
    double moveSpeed = 2.0;      // 移动速度
    double moveRange = 96.0;     // 来回范围（像素）
    double moveOriginX = 0;      // 运动原点 X（在setPos后自动记录）
    double moveOriginY = 0;
    int moveDirection = 1;       // 1 或 -1，控制当前方向
    bool originSet = false;      // 是否已记录原点

    void enableMove(MoveAxis axis, double speed, double range);

private:
    TileType m_type;
};
#endif // TILE_H