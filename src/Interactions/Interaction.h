#ifndef MERKATOR_INTERACTION_H_
#define MERKATOR_INTERACTION_H_

class Node;
class MainWindow;
class Projection;
class Node;
class Way;

class QMouseEvent;
class QPaintEvent;
class QPainter;

#include "MapView.h"
#include "InfoDock.h"
#include "FeaturesDock.h"
#include "Document.h"
#include "Features.h"

#include <QtCore/QObject>
#include <QtCore/QTime>
#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QMouseEvent>
#include <QList>

#include <algorithm>

#define XY_TO_COORD(x)  theMain->view()->fromView(x)
#define COORD_TO_XY(x)  theMain->view()->toView(x)

class Interaction : public QObject
{
    Q_OBJECT
public:
    Interaction(MainWindow* aMain);
    virtual ~Interaction();

    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* ev);

    virtual void paintEvent(QPaintEvent* anEvent, QPainter& thePainter);
    virtual QString toHtml() = 0;

    MapView* view();
    Document* document();
    MainWindow* main();
    bool panning() const;

    virtual void updateSnap(QMouseEvent* event);
    Feature* lastSnap();

protected:
    MainWindow* theMain;
    bool Panning;
    QPoint FirstPan;
    QPoint LastPan;

    Feature* LastSnap;
    QList<Feature*> NoSnap;
    bool SnapActive;
    bool NoSelectPoints;
    bool NoSelectWays;
    bool NoSelectRoads;
    bool NoSelectVirtuals;

    QList<Feature*> StackSnap;
    QList<Feature*> SnapList;
    int curStackSnap;

signals:
    void requestCustomContextMenu(const QPoint & pos);
    void featureSnap(Feature*);

protected:
    bool Dragging;
    Coord StartDrag;
    Coord EndDrag;
};

class FeatureSnapInteraction : public Interaction
{
public:
    FeatureSnapInteraction(MainWindow* aMain);

    virtual void paintEvent(QPaintEvent* anEvent, QPainter& thePainter);

    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseReleaseEvent(QMouseEvent * event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);

    virtual void snapMousePressEvent(QMouseEvent * , Feature*);
    virtual void snapMouseReleaseEvent(QMouseEvent * , Feature*);
    virtual void snapMouseMoveEvent(QMouseEvent* , Feature*);
    virtual void snapMouseDoubleClickEvent(QMouseEvent* , Feature*);

    void activateSnap(bool b);
    void addToNoSnap(Feature* F);
    void addToNoSnap(QList<Feature *> Fl);
    void clearNoSnap();
    void clearSnap();
    void clearLastSnap();
    QList<Feature*> snapList();
    void addSnap(Feature* aSnap);
    void setSnap(QList<Feature*> aSnapList);
    void nextSnap();
    void previousSnap();

    void setDontSelectPoints(bool b);
    void setDontSelectRoads(bool b);
    void setDontSelectVirtual(bool b);

#ifndef _MOBILE
    virtual QCursor cursor() const;
#endif
private:
    QCursor handCursor;
    QCursor grabCursor;
    QCursor defaultCursor;
    QCursor warningCursor;

protected:
};

#endif


