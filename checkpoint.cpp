#include "checkpoint.h"

Checkpoint::Checkpoint() {
    uncheckedPix = QPixmap(":/tu/checkpoint.png");
    checkedPix = QPixmap(":/tu/checkpoint_check.png");
    setPixmap(uncheckedPix);
}

void Checkpoint::updateLogic() {
    // 静态道具，无需每帧逻辑
}

void Checkpoint::activate() {
    if (!isActivated) {
        isActivated = true;
        setPixmap(checkedPix);
    }
}
