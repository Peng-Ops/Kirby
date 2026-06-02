#include "goal.h"
#include <QPixmap>

Goal::Goal() {
    QPixmap flag(":/tu/Waving Flag Red.gif");
    if (!flag.isNull()) {
        setPixmap(flag);
    } else {
        // 回退：金色方块
        QPixmap pix(48, 96);
        pix.fill(QColor(255, 215, 0));
        setPixmap(pix);
    }
    setScale(2.0);
}
