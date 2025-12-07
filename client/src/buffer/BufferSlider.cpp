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

    // Get groove rect (the track area)
    QRect groove = style()->subControlRect(QStyle::CC_Slider, &opt, 
                                        QStyle::SC_SliderGroove, this);

    // Calculate positions
    int total = maximum() - minimum();
    if (total == 0) return;

    int currentPos = (value() - minimum()) * groove.width() / total;
    int bufferedPos = (bufferedValue_ - minimum()) * groove.width() / total;

    // Draw buffered region (gray) from current position to buffered position
    if (bufferedPos > currentPos) {
        QRect bufferedRect = groove;
        bufferedRect.setLeft(groove.left() + currentPos);
        bufferedRect.setRight(groove.left() + bufferedPos);
        bufferedRect.setHeight(4); // Match slider height
        bufferedRect.moveTop(groove.center().y() - 2);

        // Gray with transparency
        painter.fillRect(bufferedRect, QColor(128, 128, 128, 180));
    }
}