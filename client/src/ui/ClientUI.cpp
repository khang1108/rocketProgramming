#include "ui/ClientUI.hpp"
#include "utils/Logger.hpp"

#include <QApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>

#include <chrono>
#include <iostream>
#include <string>

ClientUI::ClientUI(const std::string& serverIP, int serverPort, const std::string& videoFile,
                   int clientRTPPort, QWidget* parent)
    : QWidget(parent),
      serverIP_(serverIP),
      serverPort_(serverPort),
      videoFile_(videoFile),
      clientRTPPort_(clientRTPPort),
      initialized_(false),
      state_(State::INIT),
      videoLabel_(nullptr),
      statusLabel_(nullptr),
      setupButton_(nullptr),
      playButton_(nullptr),
      pauseButton_(nullptr),
      teardownButton_(nullptr),
      statusTimer_(nullptr),
      frameTimer_(nullptr),
      timelineSlider_(nullptr),
      currentTimeLabel_(nullptr),
      totalTimeLabel_(nullptr),
      prebufferBar_(nullptr),
      prebufferLabel_(nullptr),
      isSeekingTimeline_(false),
      totalFrames_(0),
      currentFrame_(0),
      bufferedFrame_(0),
      prebufferReady_(false),
      framesThisSecond_(0),
      fps_(0) {
    auto* mainLayout = new QVBoxLayout(this);
    auto* buttonLayout = new QHBoxLayout();

    setupButton_ = new QPushButton("SETUP");
    playButton_ = new QPushButton("PLAY");
    pauseButton_ = new QPushButton("PAUSE");
    teardownButton_ = new QPushButton("TEARDOWN");

    buttonLayout->addWidget(setupButton_);
    buttonLayout->addWidget(playButton_);
    buttonLayout->addWidget(pauseButton_);
    buttonLayout->addWidget(teardownButton_);

    videoLabel_ = new QLabel("Client Previewer");
    videoLabel_->setAlignment(Qt::AlignCenter);
    videoLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    videoLabel_->setMinimumSize(640, 480);
    videoLabel_->setStyleSheet("background-color: black; color: white;");

    QHBoxLayout* prebufferLayout = new QHBoxLayout();

    prebufferLabel_ = new QLabel("Buffer:");
    prebufferBar_ = new QProgressBar();
    // Default until we know totalFrames_ after SETUP
    prebufferBar_->setMaximum(std::max(1, static_cast<int>(PREBUFFER_FRAMES)));
    prebufferBar_->setValue(0);
    prebufferBar_->setTextVisible(true);
    prebufferBar_->setFormat("%v/%m frames");
    prebufferBar_->setMaximumHeight(20);

    prebufferLayout->addWidget(prebufferLabel_);
    prebufferLayout->addWidget(prebufferBar_, 1);

    mainLayout->addLayout(prebufferLayout);

    QHBoxLayout* timelineLayout = new QHBoxLayout();

    // currentTimeLabel_ = new QLabel("00:00");
    // currentTimeLabel_->setMinimumWidth(50);

    timelineSlider_ = new BufferedSlider(Qt::Horizontal);
    timelineSlider_->setMinimum(0);
    timelineSlider_->setMaximum(PREBUFFER_FRAMES);  // Will update later
    timelineSlider_->setValue(0);
    timelineSlider_->setBufferedValue(0);
    timelineSlider_->setEnabled(false);

    timelineSlider_->setStyleSheet("QSlider::groove:horizontal {"
                                   "    border: 1px solid #999;"
                                   "    height: 6px;"
                                   "    background: #333;"  // Dark gray: chưa load
                                   "    margin: 2px 0;"
                                   "    border-radius: 3px;"  // Bo tròn góc
                                   "}"
                                   "QSlider::handle:horizontal {"
                                   "    background: #2196F3;"  // Blue handle
                                   "    border: 2px solid #1976D2;"
                                   "    width: 14px;"  // Tăng size handle
                                   "    height: 14px;"
                                   "    margin: -5px 0;"
                                   "    border-radius: 7px;"
                                   "}"
                                   "QSlider::sub-page:horizontal {"  // Phần đã play
                                   "    background: #2196F3;"        // Blue: đã play
                                   "    border-radius: 3px;"
                                   "}");

    // totalTimeLabel_ = new QLabel("00:00");
    // totalTimeLabel_->setMinimumWidth(50);

    // timelineLayout->addWidget(currentTimeLabel_);
    timelineLayout->addWidget(timelineSlider_, 1);
    // timelineLayout->addWidget(totalTimeLabel_);

    mainLayout->addLayout(timelineLayout);

    statusLabel_ = new QLabel("[INIT] Ready");
    statusLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(videoLabel_, 1);
    mainLayout->addWidget(statusLabel_);

    setLayout(mainLayout);
    setWindowTitle("RTSP Video Client");

    connect(setupButton_, &QPushButton::clicked, this, &ClientUI::onSetupClicked);
    connect(playButton_, &QPushButton::clicked, this, &ClientUI::onPlayClicked);
    connect(pauseButton_, &QPushButton::clicked, this, &ClientUI::onPauseClicked);
    connect(teardownButton_, &QPushButton::clicked, this, &ClientUI::onTeardownClicked);

    statusTimer_ = new QTimer(this);
    connect(statusTimer_, &QTimer::timeout, this, &ClientUI::updateStatus);

    frameTimer_ = new QTimer(this);
    connect(frameTimer_, &QTimer::timeout, this, &ClientUI::updateFrame);

    statusTimer_->start(1000);
    frameTimer_->start(45);

    connect(timelineSlider_, &QSlider::sliderPressed, this, &ClientUI::onTimelineSliderPressed);
    connect(timelineSlider_, &QSlider::sliderReleased, this, &ClientUI::onTimelineSliderReleased);
    connect(timelineSlider_, &QSlider::valueChanged, this, &ClientUI::onTimelineValueChanged);

    updateButtonStates();  // Set initial button states
}

ClientUI::~ClientUI() {
    if (rtpReceiver_) {
        rtpReceiver_->stop();
    }
}

bool ClientUI::initialize() {
    try {
        Logger::getInstance().log(LogLevel::INFO, "Initializing Client UI ...");

        rtspClient_ = std::make_unique<RTSPClient>(serverIP_, serverPort_);

        // Buffer lớn hơn để chứa nhiều frames, đảm bảo playback mượt
        // 500 frames = ~20 giây video @ 25 FPS
        frameBuffer_ = std::make_unique<FrameBuffer>(500);
        frameReassembler_ = std::make_unique<FrameReassembler>(frameBuffer_.get());

        rtpReceiver_ = std::make_unique<RTPReceiver>(clientRTPPort_, frameReassembler_.get());

        initialized_ = true;
        prebufferReady_ = false;
        framesThisSecond_ = 0;
        fps_ = 0;

        Logger::getInstance().log(LogLevel::INFO, "Initialized Client UI successfully");

        return true;
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR,
                                  "Failed to initialize Client UI: " + std::string(e.what()));
        return false;
    }
}

void ClientUI::onTimelineValueChanged(int value) {}

void ClientUI::onSetupClicked() {
    if (!initialized_) {
        std::cerr << "[SETUP] ClientUI not initialized\n";
        return;
    }

    try {
        Logger::getInstance().log(LogLevel::INFO, "SETUP button clicked");
        std::cout << "\n[SETUP] Connecting to server ...\n";

        bool success = rtspClient_->sendSetup(videoFile_, clientRTPPort_);

        if (success) {
            currentFrame_ = 0;
            prebufferReady_ = false;
            if (frameBuffer_)
                frameBuffer_->clear();

            std::string sessionId = rtspClient_->getSessionId();
            state_ = State::READY;  // INIT → READY
            updateButtonStates();

            totalFrames_ = rtspClient_->getTotalFrames();
            // Align prebuffer progress bar max with total frames (fallback to default)
            if (prebufferBar_) {
                int prebufferMax =
                    (totalFrames_ > 0) ? totalFrames_ : static_cast<int>(PREBUFFER_FRAMES);
                prebufferBar_->setMaximum(prebufferMax);
            }

            std::cout << "[ClientUI] totalFrames_ = " << totalFrames_ << "\n";

            timelineSlider_->setEnabled(true);
            // timelineSlider_->setMaximum(totalFrames_);

            std::cout << "[SETUP] Success! Session ID: " << sessionId << "\n";
            Logger::getInstance().log(LogLevel::INFO,
                                      "SETUP successfully, Session ID: " + sessionId);
        } else {
            std::cout << "[SETUP] Failed please check again\n";
            Logger::getInstance().log(LogLevel::ERROR, "SETUP Failed");
        }
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, std::string("SETUP Failed: ") + e.what());
        std::cerr << "[SETUP] failed: " << e.what() << '\n';
    }
}
void ClientUI::onPlayClicked() {
    if (!initialized_) {
        std::cerr << "[PLAY] ClientUI not initialized\n";
        return;
    }

    try {
        Logger::getInstance().log(LogLevel::INFO, "PLAY button clicked");
        std::cout << "\n[PLAY] Playing video ...\n";

        // Kiểm tra xem video đã phát hết chưa để quyết định có restart hay không
        bool needRestart = (currentFrame_ >= totalFrames_);

        if (needRestart) {
            LOG_INFO("[PLAY] Video ended, requesting restart from server");
        } else {
            LOG_INFO("[PLAY] Video not ended (buffer still has frames or paused mid-stream), no "
                     "restart needed");
        }

        bool success = rtspClient_->sendPlay(needRestart);

        if (success) {
            // Reset playback position if video ended
            if (needRestart) {
                currentFrame_ = 0;
                LOG_INFO("[PLAY] Client reset currentFrame to 0");

                // Clear "End of Video" message
                if (videoLabel_) {
                    videoLabel_->clear();
                    videoLabel_->setText("Loading video...");
                    videoLabel_->setStyleSheet("background-color: black; color: white;");
                }
            }

            if (rtpReceiver_ && !rtpReceiver_->isRunning()) {
                rtpReceiver_->start();
            }

            if (frameBuffer_->size() <= 0)
                prebufferReady_ = false;

            fps_ = 0;
            fpsStartTime_ = std::chrono::steady_clock::now();

            state_ = State::PLAYING;  // READY → PLAYING
            std::cout << "[STATE] Changed to PLAYING - PAUSE button should be enabled\n";
            updateButtonStates();

            std::cout << "[PLAY] Success!\n";
            Logger::getInstance().log(LogLevel::INFO, "PLAY successfully");
        } else {
            std::cout << "[PLAY] Failed please check again\n";
            Logger::getInstance().log(LogLevel::ERROR, "PLAY Failed");
        }
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, std::string("PLAY Failed: ") + e.what());
        std::cerr << "[PLAY] failed: " << e.what() << '\n';
    }
}
void ClientUI::onPauseClicked() {
    std::cout << "\n[DEBUG] PAUSE button clicked! Current state: " << static_cast<int>(state_)
              << "\n";

    if (!initialized_) {
        std::cerr << "[PAUSE] ClientUI not initialized\n";
        return;
    }

    try {
        Logger::getInstance().log(LogLevel::INFO, "PAUSE button clicked");
        std::cout << "\n[PAUSE] Pausing ...\n";

        bool success = rtspClient_->sendPause();

        if (success) {
            std::cout << "[STATE] Changing PLAYING → READY (pause requested)\n";
            state_ = State::READY;  // PLAYING → READY
            updateButtonStates();

            std::cout << "[PAUSE] Success!\n";
            Logger::getInstance().log(LogLevel::INFO, "PAUSE successfully");
        } else {
            std::cout << "[PAUSE] Failed please check again\n";
            Logger::getInstance().log(LogLevel::ERROR, "PAUSE Failed");
        }
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, std::string("PAUSE Failed: ") + e.what());
        std::cerr << "[PAUSE] failed: " << e.what() << '\n';
    }
}
void ClientUI::onTeardownClicked() {
    if (!initialized_) {
        std::cerr << "[TEARDOWN] ClientUI not initialized\n";
        return;
    }

    try {
        Logger::getInstance().log(LogLevel::INFO, "TEARDOWN button clicked");
        std::cout << "\n[TEARDOWN] Ending session ...\n";

        if (rtpReceiver_)
            rtpReceiver_->stop();
        if (frameBuffer_) {
            frameBuffer_->clear();
        }

        prebufferReady_ = false;
        currentFrame_ = 0;
        fps_ = 0;

        if (rtspClient_)
            rtspClient_->sendTeardown();

        rtspClient_.reset();
        rtpReceiver_.reset();
        frameReassembler_.reset();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        try {
            rtspClient_ = std::make_unique<RTSPClient>(serverIP_, serverPort_);
            frameReassembler_ = std::make_unique<FrameReassembler>(frameBuffer_.get());
            rtpReceiver_ = std::make_unique<RTPReceiver>(clientRTPPort_, frameReassembler_.get());
        } catch (const std::exception& e) {
            Logger::getInstance().log(
                LogLevel::ERROR,
                std::string("Failed to recreate RTSP client after TEARDOWN: ") + e.what());
            state_ = State::INIT;
            updateButtonStates();
            statusLabel_->setText("Error: cannot reconnect to server");
            return;
        }
        // 5. Reset state → INIT và update nút
        state_ = State::INIT;
        updateButtonStates();

        std::cout << "[TEARDOWN] Success! Ready for new SETUP\n";
        Logger::getInstance().log(LogLevel::INFO, "TEARDOWN done, RTSP client recreated");
    } catch (const std::exception& e) {
        Logger::getInstance().log(LogLevel::ERROR, std::string("TEARDOWN Failed: ") + e.what());
        std::cerr << "[TEARDOWN] failed: " << e.what() << '\n';
    }
}

QString ClientUI::makeStatusString() const {
    std::string state;
    switch (state_) {
        case State::INIT:
            state = "INIT";
            break;
        case State::READY:
            state = "READY";
            break;
        case State::PLAYING:
            state = "PLAYING";
            break;
    }

    double packetLoss = 0.0;
    uint64_t packetsReceived = 0;
    int bufferSize = 0;

    if (rtpReceiver_) {
        packetLoss = rtpReceiver_->getPacketLossPercentage();
        packetsReceived = rtpReceiver_->getPacketReceived();
    }

    if (frameBuffer_)
        bufferSize = frameBuffer_->size();

    QString qs;
    qs.asprintf("[%s] FPS: %d | Buffer: %d frames | Packets: %llu | Loss: %.2f%%", state.c_str(),
                fps_, bufferSize, static_cast<unsigned long long>(packetsReceived), packetLoss);
    return qs;
}

void ClientUI::updateButtonStates() {
    // RTSP State Machine:
    // INIT → SETUP → READY → PLAY → PLAYING → PAUSE → READY
    //                 READY/PLAYING → TEARDOWN → INIT

    // DEBUG: Log mỗi lần update

    std::cout << "updateButtonStates: state_ = " << static_cast<int>(state_) << "\n";

    static int callCount = 0;
    callCount++;
    if (callCount % 25 == 1) {
        std::string stateName = (state_ == State::INIT    ? "INIT"
                                 : state_ == State::READY ? "READY"
                                                          : "PLAYING");
        std::cout << "[updateButtonStates] State: " << stateName << "\n";
    }

    switch (state_) {
        case State::INIT:
            // Only SETUP is allowed
            setupButton_->setEnabled(true);
            playButton_->setEnabled(false);
            pauseButton_->setEnabled(false);
            teardownButton_->setEnabled(false);
            break;

        case State::READY:
            // Can PLAY or TEARDOWN
            setupButton_->setEnabled(false);
            playButton_->setEnabled(true);
            pauseButton_->setEnabled(false);
            teardownButton_->setEnabled(true);
            break;

        case State::PLAYING:
            // Can PAUSE or TEARDOWN
            setupButton_->setEnabled(false);
            playButton_->setEnabled(false);
            pauseButton_->setEnabled(true);
            teardownButton_->setEnabled(true);
            break;
    }
}

void ClientUI::updateStatus() {
    fps_ = framesThisSecond_;
    framesThisSecond_ = 0;
    statusLabel_->setText(makeStatusString());
}

void ClientUI::updateFrame() {
    if (!initialized_ || !frameBuffer_ || !videoLabel_ || !statusLabel_)
        return;

    // Static counters for buffer empty tracking (moved to function scope)
    static bool shownEmptyWarning = false;
    static int emptyCount = 0;

    try {
        updatePrebufferIndicator();
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in updatePrebufferIndicator: " + std::string(e.what()));
        return;
    }

    if (state_ != State::PLAYING) {
        return;
    }

    size_t currentBufferSize = frameBuffer_->size();

    if (currentBufferSize == 0) {
        // Mark as end-of-video if:
        // 1. We've displayed all frames (currentFrame_ >= totalFrames_), OR
        bool isEndOfVideo = (totalFrames_ > 0 && (currentFrame_ >= totalFrames_));

        std::cout << "[DEBUG] Buffer empty - currentFrame_=" << currentFrame_
                  << ", totalFrames_=" << totalFrames_
                  << ", isEndOfVideo=" << (isEndOfVideo ? "YES" : "NO") << "\n";

        if (isEndOfVideo) {
            std::cout << "[BUFFER] End of stream detected - STOPPING\n";
            LOG_INFO("[BUFFER] Video playback completed");

            std::cout << "[STATE] Changing PLAYING → READY (end of video)\n";

            // Set currentFrame_ to totalFrames_ to ensure needRestart = true when PLAY is clicked
            currentFrame_ = totalFrames_;

            state_ = State::READY;
            prebufferReady_ = false;
            fps_ = 0;

            if (rtpReceiver_)
                rtpReceiver_->stop();
            if (frameBuffer_)
                frameBuffer_->clear();

            statusLabel_->setText("Video ended. Click PLAY to watch again.");

            if (videoLabel_) {
                videoLabel_->setText("End of Video - Click PLAY to restart");
                videoLabel_->setStyleSheet("background-color: black; color: white;");
                videoLabel_->setAlignment(Qt::AlignCenter);
                videoLabel_->setPixmap(QPixmap());
            }

            updateButtonStates();
            return;
        }

        if (!shownEmptyWarning) {
            std::cout << "[BUFFER] Buffer empty! Waiting for data...\n";
            LOG_WARN("[BUFFER] Buffer empty! Waiting for data...");
            shownEmptyWarning = true;
            emptyCount = 0;
        }

        emptyCount++;
        if (emptyCount > 50) {
            // Timeout case: buffer empty for 2 seconds (network issue or unexpected end)
            std::cout << "[BUFFER] Buffer empty for 2 seconds - TIMEOUT\n";
            LOG_INFO("[BUFFER] Buffer empty for 2 seconds - stopping playback (timeout)");

            std::cout << "[STATE] Changing PLAYING → READY (timeout)\n";

            // Set currentFrame_ to totalFrames_ to ensure needRestart = true when PLAY is clicked
            currentFrame_ = totalFrames_;

            // Change state to READY (session still active for replay)
            state_ = State::READY;
            prebufferReady_ = false;
            emptyCount = 0;

            // Stop RTP receiver
            if (rtpReceiver_) {
                std::cout << "[BUFFER] Stopping RTP receiver...\n";
                rtpReceiver_->stop();
            }

            // Clear buffer
            if (frameBuffer_) {
                std::cout << "[BUFFER] Clearing frame buffer...\n";
                frameBuffer_->clear();
            }

            statusLabel_->setText("Video ended. Click PLAY to watch again.");

            if (videoLabel_) {
                videoLabel_->setText("End of Video - Click PLAY to restart");
                videoLabel_->setStyleSheet("background-color: black; color: white;");
                videoLabel_->setAlignment(Qt::AlignCenter);
                videoLabel_->setPixmap(QPixmap());
            }

            updateButtonStates();
            return;
        } else {
            statusLabel_->setText(QString("Rebuffering... 0/%1 frames (%2s)")
                                      .arg(PREBUFFER_FRAMES)
                                      .arg(emptyCount * 40 / 1000.0, 0, 'f', 1));
        }

        prebufferReady_ = false;
        return;
    }

    if (!prebufferReady_) {
        size_t bufferSize = frameBuffer_->size();

        if (bufferSize >= PREBUFFER_FRAMES) {
            prebufferReady_ = true;
            std::cout << "[BUFFER] Prebuffer ready: " << bufferSize << " frames\n";
            LOG_INFO("[BUFFER] Prebuffer ready: " + std::to_string(bufferSize) + " frames");
        } else {
            double percentage = bufferSize * 100.0 / PREBUFFER_FRAMES;
            statusLabel_->setText(QString("Buffering... %1/%2 frames (%3%)")
                                      .arg(bufferSize)
                                      .arg(PREBUFFER_FRAMES)
                                      .arg(percentage, 0, 'f', 1));
            return;
        }
    }

    const size_t LOW_BUFFER_THRESHOLD = 5;
    if (currentBufferSize < LOW_BUFFER_THRESHOLD) {
        static int lowBufferCount = 0;
        lowBufferCount++;

        if (lowBufferCount % 25 == 1) {
            std::cout << "[BUFFER] Buffer low (" << currentBufferSize
                      << " frames), continuing playback but may stutter\n";
            LOG_WARN("[BUFFER] Buffer low (" + std::to_string(currentBufferSize) +
                     " frames), continuing playback but may stutter");
        }

        statusLabel_->setText(
            QString("[WARNING] Low Buffer: %1 frames (loading...)").arg(currentBufferSize));
    }

    if (currentBufferSize >= 20) {
        frameTimer_->setInterval(38);
    } else if (currentBufferSize >= 10) {
        frameTimer_->setInterval(40);
    } else if (currentBufferSize >= 5) {
        frameTimer_->setInterval(45);
    } else {
        frameTimer_->setInterval(50);
    }

    std::vector<uint8_t> frame;
    if (!frameBuffer_->tryPop(frame)) {
        return;
    }

    if (frame.empty()) {
        LOG_WARN("[BUFFER] Popped empty frame data");
        return;
    }

    // WRAP JPEG decoding trong try-catch để tránh crash
    try {
        QImage image;
        if (!image.loadFromData(frame.data(), static_cast<int>(frame.size()), "JPEG")) {
            LOG_ERROR("Failed to decode JPEG frame");
            return;
        }

        if (!videoLabel_) {
            LOG_ERROR("videoLabel_ is null!");
            return;
        }

        QPixmap pixmap = QPixmap::fromImage(
            image.scaled(videoLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

        videoLabel_->setPixmap(pixmap);
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in frame rendering: " + std::string(e.what()));
        return;
    } catch (...) {
        LOG_ERROR("Unknown exception in frame rendering");
        return;
    }

    currentFrame_++;
    framesThisSecond_++;

    // Reset empty buffer tracking counters after successfully displaying a frame
    shownEmptyWarning = false;
    emptyCount = 0;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - fpsStartTime_).count();

    if (elapsed >= 1) {
        fps_ = framesThisSecond_;
        framesThisSecond_ = 0;
        fpsStartTime_ = now;
        std::cout << "[STATUS] FPS: " << fps_ << " | Buffer: " << frameBuffer_->size() << " frames"
                  << " | Frame: " << currentFrame_ << "/" << totalFrames_
                  << " | State: " << (state_ == State::PLAYING ? "PLAYING" : "READY/INIT") << "\n";
    }

    updateTimeline();
}

void ClientUI::onTimelineSliderPressed() {
    isSeekingTimeline_ = true;
    LOG_INFO("[ClientUI] Timeline seek started");
}

void ClientUI::onTimelineSliderReleased() {
    isSeekingTimeline_ = false;

    int targetFrame = timelineSlider_->value();
    LOG_INFO("Timeline seek to frame: " + std::to_string(targetFrame));

    std::cout << "USER SEEK: " << currentFrame_ << " -> " << targetFrame << "\n";
    LOG_INFO("Timeline seek to frame: " + std::to_string(targetFrame));

    currentFrame_ = targetFrame;
}

void ClientUI::updateTimeline() {
    // NULL SAFETY: Kiểm tra slider tồn tại
    if (!timelineSlider_ || !frameBuffer_) {
        return;
    }

    if (isSeekingTimeline_ || state_ != State::PLAYING) {
        return;
    }

    if (!isSeekingTimeline_ && state_ == State::PLAYING) {
        try {
            timelineSlider_->blockSignals(true);

            int maxFrames = timelineSlider_->maximum();
            if (totalFrames_ > 0 && maxFrames != totalFrames_) {
                timelineSlider_->setMinimum(0);
                timelineSlider_->setMaximum(totalFrames_);
                maxFrames = totalFrames_;
            }
            if (maxFrames <= 0) {
                maxFrames = 1000;
                timelineSlider_->setMinimum(0);
                timelineSlider_->setMaximum(maxFrames);
            }

            int clampedCurrent = std::min(std::max(0, currentFrame_), maxFrames);
            timelineSlider_->setValue(clampedCurrent);

            size_t bufferSize = frameBuffer_->size();
            bufferedFrame_ = clampedCurrent + static_cast<int>(bufferSize);
            int bufferedPos = std::min(bufferedFrame_, maxFrames);
            timelineSlider_->setBufferedValue(bufferedPos);

            static int logCount = 0;
            if (++logCount % 25 == 0) {
                std::cout << "[TIMELINE] Current: " << clampedCurrent
                          << " | Buffered: " << bufferedPos << " | Slider max: " << maxFrames
                          << " | Buffer size: " << bufferSize << " frames\n";
            }

            timelineSlider_->blockSignals(false);
        } catch (const std::exception& e) {
            timelineSlider_->blockSignals(false);
            LOG_ERROR("Exception in updateTimeline: " + std::string(e.what()));
        } catch (...) {
            timelineSlider_->blockSignals(false);
            LOG_ERROR("Unknown exception in updateTimeline");
        }
    }
}

void ClientUI::updatePrebufferIndicator() {
    if (!prebufferBar_ || !frameBuffer_)
        return;

    int bufferSize = static_cast<int>(frameBuffer_->size());

    prebufferBar_->setValue(bufferSize);

    if (bufferSize >= (int)PREBUFFER_FRAMES) {
        // GREEN: Healthy
        prebufferBar_->setStyleSheet(
            "QProgressBar { border: 1px solid #4CAF50; background-color: #1E1E1E; "
            "text-align: center; color: white; }"
            "QProgressBar::chunk { background-color: #4CAF50; }");
    } else if (bufferSize >= 5) {
        // YELLOW: Low but OK
        prebufferBar_->setStyleSheet(
            "QProgressBar { border: 1px solid #FFC107; background-color: #1E1E1E; "
            "text-align: center; color: white; }"
            "QProgressBar::chunk { background-color: #FFC107; }");
    } else {
        // RED: Critical
        prebufferBar_->setStyleSheet(
            "QProgressBar { border: 1px solid #F44336; background-color: #1E1E1E; "
            "text-align: center; color: white; }"
            "QProgressBar::chunk { background-color: #F44336; }");
    }
}

QString ClientUI::formatTime(int seconds) const {
    int mins = seconds / 60;
    int secs = seconds % 60;
    return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}