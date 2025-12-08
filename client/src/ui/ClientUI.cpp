#include "ui/ClientUI.hpp"
#include "utils/Logger.hpp"

#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QSlider>
#include <QProgressBar>

#include <chrono>
#include <iostream>
#include <string>

ClientUI::ClientUI(const std::string& serverIP, int serverPort, const std::string& videoFile,
                    int clientRTPPort, QWidget* parent)
    : 
        QWidget(parent),
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
    prebufferBar_->setMaximum(PREBUFFER_FRAMES);
    prebufferBar_->setValue(0);
    prebufferBar_->setTextVisible(true);
    prebufferBar_->setFormat("%v/%m frames");
    prebufferBar_->setMaximumHeight(20);
    
    prebufferLayout->addWidget(prebufferLabel_);
    prebufferLayout->addWidget(prebufferBar_, 1);
    
    mainLayout->addLayout(prebufferLayout);

    QHBoxLayout* timelineLayout = new QHBoxLayout();
    
    currentTimeLabel_ = new QLabel("00:00");
    currentTimeLabel_->setMinimumWidth(50);
    
    timelineSlider_ = new BufferedSlider(Qt::Horizontal);
    timelineSlider_->setMinimum(0);
    timelineSlider_->setMaximum(1000); // Will update later
    timelineSlider_->setValue(0);
    timelineSlider_->setBufferedValue(0); 
    timelineSlider_->setEnabled(false);

    timelineSlider_->setStyleSheet(
        "QSlider::groove:horizontal {"
        "    border: 1px solid #999;"
        "    height: 4px;"
        "    background: #333;"
        "    margin: 2px 0;"
        "}"
        "QSlider::handle:horizontal {"
        "    background: #1E88E5;"
        "    border: 1px solid #1565C0;"
        "    width: 12px;"
        "    margin: -5px 0;"
        "    border-radius: 6px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "    background: #1E88E5;"  // Current playback position (blue)
        "    height: 4px;"
        "}"
    );

    totalTimeLabel_ = new QLabel("00:00");
    totalTimeLabel_->setMinimumWidth(50);
    
    timelineLayout->addWidget(currentTimeLabel_);
    timelineLayout->addWidget(timelineSlider_, 1);
    timelineLayout->addWidget(totalTimeLabel_);
    
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
    frameTimer_->start(40);

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
            std::string sessionId = rtspClient_->getSessionId();
            state_ = State::READY;  // INIT → READY
            updateButtonStates();

            timelineSlider_->setEnabled(true);

            totalFrames_ = 10000; // Placeholder
            timelineSlider_->setMaximum(totalFrames_);
            
            int totalSeconds = totalFrames_ / 25; // Assume 25 FPS
            totalTimeLabel_->setText(formatTime(totalSeconds));

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

        bool success = rtspClient_->sendPlay();

        if (success) {
            rtpReceiver_->start();
            prebufferReady_ = false;
            framesThisSecond_ = 0;
            fps_ = 0;
            currentFrame_ = 0;
            fpsStartTime_ = std::chrono::steady_clock::now();

            state_ = State::PLAYING;  // READY → PLAYING
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
    if (!initialized_) {
        std::cerr << "[PAUSE] ClientUI not initialized\n";
        return;
    }

    try {
        Logger::getInstance().log(LogLevel::INFO, "PAUSE button clicked");
        std::cout << "\n[PAUSE] Pausing ...\n";

        bool success = rtspClient_->sendPause();

        if (success) {
            rtpReceiver_->stop();
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

        bool success = rtspClient_->sendTeardown();

        if (success) {
            state_ = State::INIT;  // READY/PLAYING → INIT
            updateButtonStates();

            std::cout << "[TEARDOWN] Success!\n";
            Logger::getInstance().log(LogLevel::INFO, "TEARDOWN successfully");
        } else {
            std::cout << "[TEARDOWN] Failed please check again\n";
            Logger::getInstance().log(LogLevel::ERROR, "TEARDOWN Failed");
        }
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

    if(frameBuffer_) bufferSize = frameBuffer_->size();

    QString qs;
    qs.asprintf("[%s] FPS: %d | Buffer: %d frames | Packets: %llu | Loss: %.2f%%", 
                state.c_str(), 
                fps_,
                bufferSize,
                static_cast<unsigned long long>(packetsReceived), 
                packetLoss);
    return qs;
}

void ClientUI::updateButtonStates() {
    // RTSP State Machine:
    // INIT → SETUP → READY → PLAY → PLAYING → PAUSE → READY
    //                 READY/PLAYING → TEARDOWN → INIT

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
    if (!initialized_ || !frameBuffer_)
        return;

    updatePrebufferIndicator();

    if (!prebufferReady_) {
        if (frameBuffer_->size() >= PREBUFFER_FRAMES) {
            prebufferReady_ = true;
            std::cout << "[BUFFER] Prebuffer ready: " << frameBuffer_->size() << " frames\n";
        } else {
            double percentage = frameBuffer_->size() * 100.0 / PREBUFFER_FRAMES;
            statusLabel_->setText(
                QString("Buffering... %1/%2 frames (%3%)")
                    .arg(frameBuffer_->size())
                    .arg(PREBUFFER_FRAMES)
                    .arg(percentage, 0, 'f', 1)
            );
            return; 
        }
    }

    if (frameBuffer_->size() < 5 && frameBuffer_->size() > 0 && state_ == State::PLAYING) {
        static int rebufferWarningCount = 0;
        rebufferWarningCount++;
        
        if (rebufferWarningCount % 25 == 0) {
            std::cout << "[BUFFER] Buffer low (" << frameBuffer_->size() << " frames), continuing playback but may stutter\n";
        }
        
        statusLabel_->setText(
            QString("[BUFFER] Low Buffer: %1 frames (loading...)")
                .arg(frameBuffer_->size())
        );
    }

    if (frameBuffer_->isEmpty() && state_ == State::PLAYING) {
        static bool wasEmpty = false;
        if (!wasEmpty) {
            std::cout << "[BUFFER] Buffer empty! Waiting for data...\n";
            wasEmpty = true;
        }
        
        // Reset prebufferReady để yêu cầu đợi buffer lại trước khi tiếp tục
        prebufferReady_ = false;
        
        statusLabel_->setText("⏸️ Buffering (0 frames)...");
        return; 
    }
    
    // Reset wasEmpty flag khi có data trở lại
    static bool wasEmpty = false;
    if (wasEmpty && !frameBuffer_->isEmpty()) {
        wasEmpty = false;
        std::cout << "[BUFFER] Buffer refilled, resuming playback\n";
    }

    std::vector<uint8_t> frame;
    if (!frameBuffer_->tryPop(frame)) {
        return;
    }

    if (frame.empty())
        return;

    QImage image;
    if (!image.loadFromData(frame.data(), static_cast<int>(frame.size()), "JPEG")) {
        LOG_ERROR("Failed to decode JPEG frame");
        return;
    }

    QPixmap pixmap = QPixmap::fromImage(
        image.scaled(videoLabel_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    videoLabel_->setPixmap(pixmap);

    currentFrame_++;
    framesThisSecond_++;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - fpsStartTime_).count();
    
    if (elapsed >= 1) {
        fps_ = framesThisSecond_;
        framesThisSecond_ = 0;
        fpsStartTime_ = now;
        std::cout << "[STATUS] FPS: " << fps_ 
                << " | Buffer: " << frameBuffer_->size() << " frames"
                << " | Frame: " << currentFrame_ << "\n";
    }

    updateTimeline();
}

void ClientUI::onTimelineSliderPressed()
{
    isSeekingTimeline_ = true;
    LOG_INFO("[ClientUI] Timeline seek started");
}

void ClientUI::onTimelineSliderReleased()
{
    isSeekingTimeline_ = false;

    int targetFrame = timelineSlider_->value();
    LOG_INFO("Timeline seek to frame: " + std::to_string(targetFrame));
    
    currentFrame_ = targetFrame;
}

void ClientUI::onTimelineValueChanged(int value)
{
    if(isSeekingTimeline_){
        int seconds = fps_ > 0 ? value / fps_ : 0;

        currentTimeLabel_->setText(formatTime(seconds));
    }
}

void ClientUI::updateTimeline()
{
    if(!isSeekingTimeline_ && state_ == State::PLAYING){
        if(fps_ > 0 && totalFrames_ > 0){
            timelineSlider_->setValue(currentFrame_);

            if (frameBuffer_) {
                bufferedFrame_ = currentFrame_ + frameBuffer_->size();
                timelineSlider_->setBufferedValue(std::min(bufferedFrame_, totalFrames_));
            }

            int currentSeconds = currentFrame_ / fps_;
            currentTimeLabel_->setText(formatTime(currentSeconds));
        }
    }
}

void ClientUI::updatePrebufferIndicator()
{
    if(!frameBuffer_) return;

    int bufferSize = frameBuffer_->size();
    int bufferCapacity = 500;  // Match FrameBuffer maxSize

    prebufferBar_->setMaximum(bufferCapacity);
    prebufferBar_->setValue(bufferSize);
    prebufferBar_->setFormat(QString("%v/%m frames (%p%)"));

    QString style;
    QString status;

    if (bufferSize >= (int)PREBUFFER_FRAMES) {
        style = "QProgressBar::chunk { background-color: #4CAF50; }";
        status = "Buffer: Healthy";
    } else if (bufferSize >= 5) {
        style = "QProgressBar::chunk { background-color: #FFC107; }";
        status = "Buffer: Low";
    } else if (bufferSize > 0) {
        style = "QProgressBar::chunk { background-color: #FF9800; }";
        status = "Buffer: Critical";
    } else {
        style = "QProgressBar::chunk { background-color: #F44336; }";
        status = "Buffer: Empty";
    }

    prebufferBar_->setStyleSheet(style);
    prebufferLabel_->setText(status);
}

QString ClientUI::formatTime(int seconds) const {
    int mins = seconds / 60;
    int secs = seconds % 60;
    return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}