#include "buffer/BufferSlider.hpp"
#include <QStyleOptionSlider>
#include <QPainter>

BufferedSlider::BufferedSlider(Qt::Orientation orientation, QWidget* parent)
    : QSlider(orientation, parent), bufferedValue_(0) {
}

void BufferedSlider::setBufferedValue(int value) {
    bufferedValue_ = value;
    update(); // Trigger repaint
}

int BufferedSlider::getBufferedValue() const {
    return bufferedValue_;
}

void BufferedSlider::paintEvent(QPaintEvent* event) {
    QSlider::paintEvent(event); // Draw default slider first

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QStyleOptionSlider opt;
    initStyleOption(&opt);

    // Get groove rect
    QRect groove = style()->subControlRect(QStyle::CC_Slider, &opt, 
                                        QStyle::SC_SliderGroove, this);

    // Calculate positions
    int total = maximum() - minimum();
    if (total == 0) return;

    int currentPos = (value() - minimum()) * groove.width() / total;
    int bufferedPos = (bufferedValue_ - minimum()) * groove.width() / total;

    // ✅ Draw buffered region (semi-transparent white/gray)
    if (bufferedPos > currentPos) {
        QRect bufferedRect = groove;
        bufferedRect.setLeft(groove.left() + currentPos);
        bufferedRect.setRight(groove.left() + bufferedPos);
        bufferedRect.setHeight(6);  // ✅ Match với stylesheet height
        bufferedRect.moveTop(groove.center().y() - 3);

        painter.fillRect(bufferedRect, QColor(180, 180, 180, 200));
    }
}