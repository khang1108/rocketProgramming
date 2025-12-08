#ifndef COMMON_UTILS_LOGGER_HPP
#define COMMON_UTILS_LOGGER_HPP

#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include "patterns/Singleton.hpp"

#ifdef ERROR
#undef ERROR
#endif

/**
 * @enum LogLevel
 * @brief Các mức độ logging
 */

enum class LogLevel
{
// Avoid accidental macro collision with a global DEBUG macro
#ifdef DEBUG
#undef DEBUG
#endif
    DEBUG = 0, ///< Chi tiết debug
    INFO = 1,  ///< Thông tin chung (server started, client connected)
    WARN = 2,  ///< Cảnh báo (timeout, retry)
    ERROR = 3  ///< Lỗi nghiêm trọng (connection failed, file not found)
};

/**
 * @class Logger
 * @brief Thread-safe logging system using Singleton pattern
 *
 * @details
 * Logger Singleton provides centralized logging for entire application:
 * - Thread-safe: Multiple threads can log simultaneously
 * - Dual output: Console + File
 * - Configurable log levels
 * - Automatic timestamps
 * - Format: [YYYY-MM-DD HH:MM:SS] [LEVEL] Message
 *
 * @example
 * @code
 * // Initialize logger (one time, usually in main())
 * Logger::getInstance().initialize("server.log", LogLevel::INFO);
 *
 * // Use from anywhere
 * LOG_INFO("Server started on port 8554");
 * LOG_ERROR("Failed to open file: " + filename);
 * LOG_DEBUG("Packet seq=" + std::to_string(seq));
 * @endcode
 *
 * @note Implementation uses Meyer's Singleton (thread-safe since C++11)
 */
class Logger : public Singleton<Logger> {
    friend class Singleton<Logger>;

  private:
    std::ofstream logFile_;  ///< Output file stream
    LogLevel minLevel_;      ///< Minimum level to log
    std::mutex mutex_;       ///< Thread synchronization
    bool initialized_;       ///< Initialization flag

    /**
     * @brief Private constructor (Singleton pattern)
     */
    Logger();

    /**
     * @brief Get current timestamp as string
     * @return Formatted timestamp "YYYY-MM-DD HH:MM:SS"
     */
    std::string getCurrentTime() const;

    /**
     * @brief Convert LogLevel to string
     * @param level Log level
     * @return String representation ("DEBUG", "INFO", "WARN", "ERROR")
     */
    std::string levelToString(LogLevel level) const;

  public:
    /**
     * @brief Initialize logger with file and level
     * @param logFile Path to log file (e.g., "server.log")
     * @param level Minimum log level (default: INFO)
     *
     * @details
     * - Creates/opens log file in append mode
     * - Sets minimum log level filter
     * - Thread-safe initialization
     * - Can be called multiple times (will close old file and open new)
     *
     * @note Must call initialize() before using log()
     *
     * @example
     * @code
     * Logger::getInstance().initialize("server.log", LogLevel::DEBUG);
     * @endcode
     */
    void initialize(const std::string& logFile, LogLevel level = LogLevel::INFO);

    /**
     * @brief Log a message with specified level
     * @param level Log level
     * @param message Message to log
     *
     * @details
     * - Thread-safe (uses mutex)
     * - Writes to both console and file
     * - Filters messages below minLevel_
     * - Auto-adds timestamp and level prefix
     *
     * @example
     * @code
     * Logger::getInstance().log(LogLevel::INFO, "Server started");
     * // Output: [2025-11-27 10:00:00] [INFO ] Server started
     * @endcode
     */
    void log(LogLevel level, const std::string& message);

    /**
     * @brief Set minimum log level
     * @param level New minimum level
     *
     * @details
     * Messages below this level will be ignored.
     * Useful for runtime log level adjustment.
     *
     * @example
     * @code
     * // Development: show everything
     * Logger::getInstance().setMinLevel(LogLevel::DEBUG);
     *
     * // Production: only important messages
     * Logger::getInstance().setMinLevel(LogLevel::WARN);
     * @endcode
     */
    void setMinLevel(LogLevel level);

    /**
     * @brief Check if logger is initialized
     * @return true if initialize() was called
     */
    bool isInitialized() const { return initialized_; }

    /**
     * @brief Destructor - closes log file
     */
    ~Logger();
};

// ==================== CONVENIENCE MACROS ====================

/**
 * @brief Log INFO message
 * @param msg Message string
 */
#define LOG_INFO(msg) Logger::getInstance().log(LogLevel::INFO, msg)

/**
 * @brief Log WARNING message
 * @param msg Message string
 */
#define LOG_WARN(msg) Logger::getInstance().log(LogLevel::WARN, msg)

/**
 * @brief Log ERROR message
 * @param msg Message string
 */
#define LOG_ERROR(msg) Logger::getInstance().log(LogLevel::ERROR, msg)

#endif