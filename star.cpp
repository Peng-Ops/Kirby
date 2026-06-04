#include "star.h"

Star::Star() {
    QPixmap sheet(":/tu/star.png");
    if (!sheet.isNull() && sheet.width() >= 48) {
        // 精灵图是 192x48 的4帧横排，取第一帧
        setPixmap(sheet.copy(0, 0, 48, 48));
    } else {
        setPixmap(sheet);
    }
}

void Star::updateLogic() {
    // 静态道具暂时不需要移动逻辑，留空即可
}
