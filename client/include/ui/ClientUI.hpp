#ifndef CLIENT_UI_HPP
#define CLIENT_UI_HPP

#include <QWidget>
#include <QSlider>
#include <QProgressBar>
#include <memory>
#include <string>
#include <chrono>

#include "buffer/FrameBuffer.hpp"
#include "network/RTSPClient.hpp"
#include "rtp/FrameReassembler.hpp"
#include "rtp/RTPReceiver.hpp"
#include "buffer/BufferSlider.hpp"

class QLabel;
class QPushButton;
class QTimer;

class ClientUI : public QWidget {
    Q_OBJECT

  public:
    enum class State {
        INIT,    ///< Initial state, can only SETUP
        READY,   ///< After SETUP, can PLAY or TEARDOWN
        PLAYING  ///< After PLAY, can PAUSE or TEARDOWN
    };

    ClientUI(const std::string& serverIP, int serverPort, const std::string& videoFile,
            int clientRTPPort, QWidget* parent = nullptr);
    ~ClientUI();

    bool initialize();
  private slots:
    void onSetupClicked();
    void onPlayClicked();
    void onPauseClicked();
    void onTeardownClicked();

    void updateStatus();
    void updateFrame();

    void onTimelineValueChanged(int value);
    void onTimelineSliderPressed();
    void onTimelineSliderReleased();

  private:
    void updateButtonStates();
    void updateTimeline();
    void updatePrebufferIndicator();
    QString formatTime(int seconds) const;

    std::string serverIP_;
    int serverPort_;
    std::string videoFile_;
    int clientRTPPort_;
    bool initialized_;
    State state_;  ///< Current RTSP session state

    std::unique_ptr<RTSPClient> rtspClient_;
    std::unique_ptr<FrameBuffer> frameBuffer_;
    std::unique_ptr<FrameReassembler> frameReassembler_;
    std::unique_ptr<RTPReceiver> rtpReceiver_;

    QLabel* videoLabel_;
    QLabel* statusLabel_;
    QPushButton* setupButton_;
    QPushButton* playButton_;
    QPushButton* pauseButton_;
    QPushButton* teardownButton_;
    QTimer* statusTimer_;
    QTimer* frameTimer_;

    BufferedSlider* timelineSlider_;
    QLabel* currentTimeLabel_;
    QLabel* totalTimeLabel_;
    QProgressBar* prebufferBar_;
    QLabel* prebufferLabel_;

    bool isSeekingTimeline_;
    int totalFrames_;
    int currentFrame_;
    int bufferedFrame_;
    std::chrono::steady_clock::time_point playStartTime_;

    static constexpr size_t PREBUFFER_FRAMES = 10;
    bool prebufferReady_;
    int framesThisSecond_;
    int fps_;
    std::chrono::steady_clock::time_point fpsStartTime_;

    QString makeStatusString() const;
};
#endif