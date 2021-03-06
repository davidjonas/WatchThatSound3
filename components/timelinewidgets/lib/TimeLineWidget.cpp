#include "utils/Macros"

#include "TimeLineWidget.hpp"
#include "EditController.hpp"
#include "Project.hpp"
#include "SharpLine.hpp"
#include "BufferItem.hpp"
#include "Synced.hpp"

//#include "MainWindow.h"
//#include "ui_MainWindow.h"
#include <QMouseEvent>



TimeLineWidget::TimeLineWidget(QWidget *parent)
    : QGraphicsView(parent)
    , m_seekOnDrag(false)
    , m_currentTime(0)
    , m_editMode(true)
    , m_deafToSeek(false)
{
#if 0
    m_controller = m_mainWindow->editController();
    connect(m_mainWindow,SIGNAL(storyBoardChanged()),SLOT(invalidateBackground()));
    connect(m_mainWindow,SIGNAL(samplerClock(qint64)),SLOT(setCurrentTime(qint64)));
#else
    FIXME("set TimeLine's edit controller");
    FIXME("connect TimeLine's slots");
#endif

    setScene(new QGraphicsScene(0.0,0.0,1.0,1.0,this));
    m_cursorLine = new SharpLine(scene());
    m_cursorLine->setPen( palette().color( QPalette::HighlightedText ) );
    m_cursorLine->setZValue(10);


    m_selectionRect = new QGraphicsRectItem(0,scene());
    m_selectionRect->setVisible(false);
    m_selectionRect->setZValue(5);
    m_selectionRect->setPen( Qt::NoPen );
    QColor selbg( palette().color( QPalette::Highlight ) );
    selbg = selbg.lighter();
    selbg.setAlpha(100);
    m_selectionRect->setBrush( QBrush( selbg ) );
    connect(scene(), SIGNAL(selectionChanged()), SLOT(updateSelection()));

    setLineWidth(0);
    setMidLineWidth(0);
    setCacheMode(QGraphicsView::CacheBackground);
}

void TimeLineWidget::setProject(Project * project)
{
    m_project = project;
    // connect common notifications...
    connect(project, SIGNAL(tensionChanged()), SLOT(invalidateBackground()));
    connect(project, SIGNAL(syncedItemRemoved(Synced*)), SLOT(syncedItemRemoved(Synced*)));
    connect(this, SIGNAL(dataChanged()), project, SLOT(save()));
}

void TimeLineWidget::syncedItemRemoved(Synced *synced)
{
    QList<QGraphicsItem *> items_list = items();
    foreach(QGraphicsItem * item, items_list) {
        Synced * item_synced = item->data(SYNCED).value<Synced*>();
        if (synced == item_synced) {
            scene()->removeItem(item);
            item->setSelected(false);
            onRemoved(synced, item);
            delete item;
        }
    }
}

void TimeLineWidget::resizeEvent ( QResizeEvent * /*event*/ )
{
    fitInView(0,0,1.0,1.0);
}

void TimeLineWidget::setCurrentTime(qint64 time)
{
    if (m_deafToSeek) return;

    if (time != m_currentTime) {
        m_currentTime = time;
        m_cursorLine->setX( (double)m_currentTime / project()->duration() );
    }
}

void TimeLineWidget::paintRange(QPainter * painter, qreal x, qreal w, const QColor& c)
{
    QRectF r(x, 0, w, 1);
    painter->setPen(c);
    painter->drawRect( r );
    painter->fillRect( r, QBrush(c) );
}

void TimeLineWidget::drawBackground ( QPainter * painter, const QRectF & rect )
{
    if (!project()) {
        QGraphicsView::drawBackground(painter, rect );
        return;
    }
    qreal total = project()->duration();
    qreal relX1 = 0.0f;

    QPainter::RenderHints oldHints = painter->renderHints();
    painter->setRenderHint(QPainter::Antialiasing, false);

    // alternating scene backgrounds
    QVector<QColor> colors;
    colors << palette().color( QPalette::Base ) << palette().color( QPalette::AlternateBase );
    int currentColor = 0;
    foreach(Project::Marker * marker,
            project()->getMarkers(Project::SCENE)) {
        qreal relX2 = (qreal)marker->at() / total;
        // draw a rectangle which as tall as a widget and runs from relX1 to relX2
        paintRange(painter, relX1, relX2-relX1, colors[currentColor]);
        currentColor = (currentColor+1) % 2;
        relX1 = relX2;
    }
    // the last one: form relX1 to 1.0
    paintRange(painter, relX1, 1.0f - relX1, colors[currentColor]);

    // event markers
    painter->setPen( palette().color( foregroundRole() ) );
    foreach(Project::Marker * marker,
            project()->getMarkers(Project::EVENT)) {
        qreal relX2 = (qreal)marker->at() / total;
        painter->drawLine(QPointF(relX2,0.), QPointF(relX2,1.));
    }

    painter->setPen(QColor(255,100,100,127));
    painter->drawPath( m_project->tensionCurve( 1.0, 1.0 ) );

    painter->setRenderHints(oldHints, true);
}

void TimeLineWidget::mousePressEvent ( QMouseEvent * event )
{
    QGraphicsView::mousePressEvent(event);

    doSeekOnDrag(event);
}

void TimeLineWidget::mouseMoveEvent ( QMouseEvent * event )
{
    QGraphicsView::mouseMoveEvent(event);
    doSeekOnDrag(event);
}

void TimeLineWidget::doSeekOnDrag( QMouseEvent * event )
{
    if (!project()) return;

    if (seekOnDrag()) {
        if (event->buttons() & Qt::LeftButton) {
            qint64 t = project()->duration() * event->x() / (qint64)width();

            QGraphicsItem * dragged = scene()->mouseGrabberItem();
            BufferItem * dragged_b = dynamic_cast<BufferItem *>(dragged);

            if (dragged_b) {
                t = dragged_b->buffer()->rangeStartAt();
            } else if (dragged) {
                Synced * synced = 0;
                findSynced(dragged, &synced);
                if (synced) {
                    t = synced->at();
                }
            }

            m_deafToSeek = true;
            // m_mainWindow->seek( t );
            FIXME("emit seek from time line widget");
            m_cursorLine->setX( (double)t / project()->duration() );
            m_deafToSeek = false;
        }
    }
}

void TimeLineWidget::assignSynced(QGraphicsItem *item, Synced *synced)
{
    item->setData(SYNCED, QVariant::fromValue(synced));
}

QGraphicsItem * TimeLineWidget::findSynced(QGraphicsItem *item, Synced **synced)
{
    while( item && !item->data(SYNCED).isValid() ) {
        item = item->parentItem();
    }
    if (item) {
        *synced = item->data(SYNCED).value<Synced*>();
        return item;
    } else {
        *synced = 0;
        return 0;
    }
}

Project *TimeLineWidget::project()
{
    return m_project;
}

void TimeLineWidget::updateSelection()
{
    QList<QGraphicsItem *> sel = scene()->selectedItems();
    if (sel.length() > 0) {
        if (sel[0] == m_selectionRect->parentItem())
            return;
        //float dx = 3.0 / width(), dy = 3.0 / height();
        QRectF marg = sel[0]->mapRectFromScene(0, 0, 3.0 / width(), 3.0 / height());
        QRectF r = sel[0]->boundingRect().adjusted(-marg.width(),-marg.height(),marg.width(),marg.height());
        m_selectionRect->setRect( r );
        m_selectionRect->setParentItem( sel[0] );
        m_selectionRect->setVisible(true);
    } else {
        m_selectionRect->setVisible(false);
        m_selectionRect->setParentItem(0);
    }
}

void TimeLineWidget::setEditMode(bool on)
{
    m_editMode = on;
    invalidateBackground();
}

void TimeLineWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Backspace:
    case Qt::Key_Delete:
        QList<QGraphicsItem *> selection = scene()->selectedItems();
        foreach(QGraphicsItem * i, selection) {
            Synced * synced = 0;
            QGraphicsItem * syncedItem = findSynced(i,&synced);
            if (syncedItem)
                m_project->removalRequested( synced );
        }
    }
}

