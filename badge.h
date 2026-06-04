#ifndef BADGE_H
#define BADGE_H

#include "gameobject.h"
#include <QPixmap>

class Badge : public GameObject {
public:
    enum BadgeType { PIG_SHARK, BRAIN, ICE_QUEEN };

    Badge(BadgeType type);
    void updateLogic() override;

    BadgeType badgeType;
    bool isResting = false;  // 已落地静止

private:
    static constexpr double gravity = 0.6;
    static constexpr double maxVy = 12.0;
};

#endif // BADGE_H
