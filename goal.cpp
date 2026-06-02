#include "goal.h"

Goal::Goal() {
    // 占位贴图：金色方块，用户后续替换
    QPixmap pix(48, 96);
    pix.fill(QColor(255, 215, 0));
    setPixmap(pix);
    setScale(1.5);
}
