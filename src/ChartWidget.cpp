#include "ChartWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void ChartWidget::setCandles(const QVector<Candle> &candles, Timeframe tf, int decimals)
{
    m_canvas.setBounds(rect());
    m_canvas.setCandles(candles, tf, decimals);
    update();
}

void ChartWidget::setMessage(const QString &message)
{
    m_canvas.setMessage(message);
    update();
}

void ChartWidget::setVolumeLabel(const QString &label)
{
    m_canvas.setVolumeLabel(label);
    update();
}

void ChartWidget::applyLive(const QDate &date, double open, double high, double low, double close, double volume)
{
    if (m_canvas.applyLive(date, open, high, low, close, volume))
        update();
}

void ChartWidget::setShowMA(bool on) { m_canvas.setShowMA(on); update(); }
void ChartWidget::setShowBollinger(bool on) { m_canvas.setShowBollinger(on); update(); }
void ChartWidget::setShowVolume(bool on) { m_canvas.setShowVolume(on); update(); }
void ChartWidget::setShowMACD(bool on) { m_canvas.setShowMACD(on); update(); }
void ChartWidget::setShowRSI(bool on) { m_canvas.setShowRSI(on); update(); }
void ChartWidget::setShowStochastic(bool on) { m_canvas.setShowStochastic(on); update(); }

void ChartWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    m_canvas.setBounds(rect());
    m_canvas.setStyle(palette(), font());
    m_canvas.paint(p);
}

void ChartWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    m_canvas.setBounds(rect());
}

void ChartWidget::mouseMoveEvent(QMouseEvent *e)
{
    const QPointF pos = e->position();
    m_canvas.panTo(pos.x());
    m_canvas.setHover(m_canvas.indexAt(pos.x()), pos);
    update();
}

void ChartWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_canvas.beginPan(e->position().x());
        setCursor(Qt::ClosedHandCursor);
    }
}

void ChartWidget::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_canvas.endPan();
        setCursor(Qt::CrossCursor);
    }
}

void ChartWidget::mouseDoubleClickEvent(QMouseEvent *)
{
    m_canvas.resetView();
    update();
}

void ChartWidget::wheelEvent(QWheelEvent *e)
{
    const QPointF pos = e->position();
    m_canvas.zoomSteps(pos.x(), e->angleDelta().y() / 120.0);
    m_canvas.setHover(m_canvas.indexAt(pos.x()), pos);
    update();
}

void ChartWidget::leaveEvent(QEvent *)
{
    m_canvas.clearHover();
    update();
}
