#include "utils/Logger.hpp"
<<<<<<< HEAD
#include <chrono>
#include <cstring>
#include <iomanip>

Logger::Logger() : logFile_(), minLevel_(LogLevel::INFO), mutex_(), initialized_(false) {}

std::string Logger::getCurrentTime() const {
=======
#include <iomanip>
#include <chrono>
#include <cstring>

Logger::Logger()
    : logFile_(), minLevel_(LogLevel::INFO), mutex_(), initialized_(false)
{
}

std::string Logger::getCurrentTime() const
{
>>>>>>> origin/nghia
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};

#if defined(_MSC_VER) || defined(_WIN32)
    localtime_s(&tm, &t);
#elif defined(__unix__) || defined(__APPLE__)
    localtime_r(&t, &tm);
#else
<<<<<<< HEAD
    std::tm* tmp = std::localtime(&t);
=======
    std::tm *tmp = std::localtime(&t);
>>>>>>> origin/nghia
    if (tmp)
        tm = *tmp;
#endif
    char buf[20];
    if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm) == 0)
        return std::string();
    return std::string(buf);
}

<<<<<<< HEAD
std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO ";
        case LogLevel::WARN:
            return "WARN ";
        case LogLevel::ERROR:
            return "ERROR";
        default:
            return "UNK  ";
    }
}

void Logger::initialize(const std::string& logFile, LogLevel level) {
=======
std::string Logger::levelToString(LogLevel level) const
{
    switch (level)
    {
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO ";
    case LogLevel::WARN:
        return "WARN ";
    case LogLevel::ERROR:
        return "ERROR";
    default:
        return "UNK  ";
    }
}

void Logger::initialize(const std::string &logFile, LogLevel level)
{
>>>>>>> origin/nghia
    std::lock_guard<std::mutex> lock(mutex_);

    if (logFile_.is_open())
        logFile_.close();

    logFile_.open(logFile, std::ios::app);
<<<<<<< HEAD
    if (!logFile_.is_open()) {
=======
    if (!logFile_.is_open())
    {
>>>>>>> origin/nghia
        std::cerr << "Logger: failed to open log file: " << logFile << std::endl;
    }

    minLevel_ = level;
    initialized_ = true;
}

<<<<<<< HEAD
void Logger::log(LogLevel level, const std::string& message) {
=======
void Logger::log(LogLevel level, const std::string &message)
{
>>>>>>> origin/nghia
    // Filter by minimum level
    if (static_cast<int>(level) < static_cast<int>(minLevel_))
        return;

    const std::string timestamp = getCurrentTime();
    const std::string levelStr = levelToString(level);

    std::ostringstream oss;
    oss << "[" << timestamp << "] [" << levelStr << "] " << message << std::endl;
    const std::string out = oss.str();

    std::lock_guard<std::mutex> lock(mutex_);

    // Console output
    std::cout << out;

    // File output if available
<<<<<<< HEAD
    if (logFile_.is_open()) {
=======
    if (logFile_.is_open())
    {
>>>>>>> origin/nghia
        logFile_ << out;
        logFile_.flush();
    }
}

<<<<<<< HEAD
void Logger::setMinLevel(LogLevel level) {
=======
void Logger::setMinLevel(LogLevel level)
{
>>>>>>> origin/nghia
    std::lock_guard<std::mutex> lock(mutex_);
    minLevel_ = level;
}

<<<<<<< HEAD
Logger::~Logger() {
=======
Logger::~Logger()
{
>>>>>>> origin/nghia
    std::lock_guard<std::mutex> lock(mutex_);
    if (logFile_.is_open())
        logFile_.close();
}