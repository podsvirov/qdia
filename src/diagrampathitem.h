#ifndef DIAGRAMPATHITEM_H
#define DIAGRAMPATHITEM_H

#include <QGraphicsPixmapItem>
#include <QGraphicsPathItem>

class DiagramPathItem : public QGraphicsPathItem
{
public:
    enum { Type = UserType + 6 };
    enum DiagramType { Path, Start, End, StartEnd };
    enum routingType { free, xy, yx, shortest };

    DiagramPathItem(DiagramType diagramType, QMenu *contextMenu,
        QGraphicsItem *parent = 0, QGraphicsScene *scene = 0);
    DiagramPathItem(QMenu *contextMenu,
            QGraphicsItem *parent, QGraphicsScene *scene);//constructor fuer Vererbung
    DiagramPathItem(const QJsonObject &json, QMenu *contextMenu);
    DiagramPathItem(const DiagramPathItem& diagram);//copy constructor

    DiagramPathItem* copy();
    void write(QJsonObject &json);

    void append(const QPointF point);
    void remove();
    void updateLast(const QPointF point);

    DiagramType diagramType() const
        { return myDiagramType; }

    virtual void setDiagramType(DiagramType type)
        { myDiagramType=type; }

    QPixmap image() const;
    QPixmap icon();
    QPainterPath getPath() const;
    QList<QPointF> getPoints()
        { return myPoints; }
    int type() const
        { return Type;}
    void setHandlerWidth(const qreal width)
    {
        myHandlerWidth = width;
    }
    void setRoutingType(const routingType newRoutingType);
    routingType getRoutingType(){
        return myRoutingType;
    }

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    void createPath();
    QPainterPath createArrow(QPointF p1, QPointF p2) const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
    QRectF boundingRect() const;
    QPainterPath shape() const;
    bool hasClickedOn(QPointF press_point, QPointF point) const;
    void mousePressEvent(QGraphicsSceneMouseEvent *e);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *e);
    void hoverEnterEvent(QGraphicsSceneHoverEvent *e);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *e);
    QPointF onGrid(QPointF pos);

private:
    DiagramType myDiagramType;
    routingType myRoutingType;
    QMenu *myContextMenu;
    QList<QPointF> myPoints;
    qreal len,breite;
    int mySelPoint,myHoverPoint;
    qreal myHandlerWidth;

};

#endif // DIAGRAMPATHITEM_H