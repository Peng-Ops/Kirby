#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include "gameobject.h"
#include <QPixmap>

class Checkpoint : public GameObject {
public:
    Checkpoint();
    void updateLogic() override;
    void activate();
    bool isActivated = false;

private:
    QPixmap uncheckedPix;
    QPixmap checkedPix;
};

#endif // CHECKPOINT_H
