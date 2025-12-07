#ifndef BUFFERED_SLIDER_HPP
#define BUFFERED_SLIDER_HPP

#include <QSlider>
#include <QPainter>
#include <QStyleOptionSlider>

class BufferedSlider : public QSlider {
    Q_OBJECT

public:
    explicit BufferedSlider(Qt::Orientation orientation, QWidget* parent = nullptr);
    ~BufferedSlider() = default;

    void setBufferedValue(int value);
    int getBufferedValue() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int bufferedValue_;
};

#endif