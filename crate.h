#ifndef CRATE_H
#define CRATE_H

#include "gameobject.h"
#include <QRectF>
#include <QVector>

class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

class Crate : public GameObject {
public:
    Crate();
    void updateLogic() override;
    void setWaterOverlayRects(const QVector<QRectF> &rects);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
    QVector<QRectF> waterOverlayRects;
};

#endif // CRATE_H


