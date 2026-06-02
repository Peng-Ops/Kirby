#ifndef CRATE_H
#define CRATE_H

#include "gameobject.h"

class Crate : public GameObject {
public:
    Crate();
    void updateLogic() override;
};

#endif // CRATE_H
