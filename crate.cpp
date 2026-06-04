#include "crate.h"

Crate::Crate() {
    QPixmap pix(":/tu/muxiang.png");
    if (!pix.isNull()) {
        setPixmap(pix.scaled(48, 48, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }
}

void Crate::updateLogic() {
    // 物理由 mainwindow 的 gameUpdate 统一处理
}
