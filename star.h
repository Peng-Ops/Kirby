#ifndef STAR_H
#define STAR_H

#include "gameobject.h"

class Star : public GameObject {
public:
    Star();
    void updateLogic() override;
};

#endif // STAR_H
