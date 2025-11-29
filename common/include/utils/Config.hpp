#ifndef COMMON_UTILS_CONFIG_HPP
#define COMMON_UTILS_CONFIG_HPP

#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include "patterns/Singleton.hpp"

/**
 * @class Config
 * @brief Quản lý cấu hình toàn cục của ứng dụng (Singleton pattern)
 *
 * @details
 * Config Singleton cung cấp:
 * - Centralized configuration management
 * - Load from file (key=value format) hoặc hardcoded defaults
 * - Type-safe getters: getInt(), getString(), getBool(), getDouble()
 * - Thread-safe reads
 * - Sử dụng SingletonWithInit vì cần truyền config file path
 *
c
 *
 * @example
 * @code
 * // In main.cpp - initialize once
 * Config::initialize("server.conf");
 *
 * // Use anywhere in codebase
 * auto& config = Config::getInstance();
 * int port = config.getInt("server_port", 8554);
 * std::string videoFile = config.getString("video_file");
 * bool debug = config.getBool("debug_mode", false);
 * @endcode
 */
class Config : public SingletonWithInit<Config> {
    friend class SingletonWithInit;

  private:
    std::map<std::string, std::string> configMap_;  ///< Key-value storage
    std::string configFilePath_;                    ///< Path to config file
    mutable std::mutex mutex_;                      ///< Thread synchronization

    /**
     * @brief Private constructor (Singleton pattern)
     * @param configFile Path to configuration file
     *
     * @details
     * - Attempts to load from file
     * - Falls back to hardcoded defaults if file not found
     * - Thread-safe initialization via SingletonWithInit
     */
    Config(const std::string& configFile);

    /**
     * @brief Load configuration from file
     * @param filename Path to config file
     * @return true if file loaded successfully, false otherwise
     *
     * @details
     * File format:
     * - Lines starting with # are comments
     * - Format: key=value
     * - Whitespace around = is trimmed
     * - Empty lines are ignored
     *
     * Example:
     * @code
     * # Server configuration
     * server_port=8554
     * video_file=movie.Mjpeg
     * @endcode
     */
    bool loadFromFile(const std::string& filename);

    /**
     * @brief Set default configuration values
     *
     * @details
     * Called if config file doesn't exist.
     * Provides sensible defaults for all settings.
     *
     * Default values:
     * - server_port = 8554 (RTSP default)
     * - rtp_port_start = 25000
     * - rtp_port_end = 26000
     * - video_file = "movie.Mjpeg"
     * - max_clients = 10
     * - frame_rate = 24
     * - buffer_size = 2048
     * - log_level = "INFO"
     * - log_file = "server.log"
     */
    void setDefaults();

    /**
     * @brief Print all configuration values (for debugging)
     */
    void printConfig() const;

  public:
    /**
     * @brief Initialize Config singleton with file path
     * @param configFile Path to configuration file (default: "config.conf")
     *
     * @details
     * - MUST be called before getInstance()
     * - Thread-safe initialization (std::call_once)
     * - Can only be initialized once
     * - If file doesn't exist, uses defaults
     *
     * @example
     * @code
     * // In main()
     * Config::initialize("server.conf");
     *
     * // Later, anywhere
     * int port = Config::getInstance().getInt("server_port");
     * @endcode
     *
     * @throws std::runtime_error if already initialized
     */
    static void initialize(const std::string& configFile = "config.conf") {
        std::call_once(initFlag_, [&configFile]() { instance_.reset(new Config(configFile)); });
    }

    // ==================== TYPE-SAFE GETTERS ====================

    /**
     * @brief Get integer value from config
     * @param key Configuration key
     * @param defaultValue Value to return if key not found
     * @return Integer value
     *
     * @details
     * - Returns defaultValue if key doesn't exist
     * - Returns defaultValue if value cannot be parsed as int
     * - Thread-safe
     *
     * @example
     * @code
     * int port = config.getInt("server_port", 8554);
     * int maxClients = config.getInt("max_clients"); // 0 if not found
     * @endcode
     */
    int getInt(const std::string& key, int defaultValue = 0) const;

    /**
     * @brief Get string value from config
     * @param key Configuration key
     * @param defaultValue Value to return if key not found
     * @return String value
     *
     * @example
     * @code
     * std::string videoFile = config.getString("video_file", "default.Mjpeg");
     * std::string logLevel = config.getString("log_level", "INFO");
     * @endcode
     */
    std::string getString(const std::string& key, const std::string& defaultValue = "") const;

    /**
     * @brief Get boolean value from config
     * @param key Configuration key
     * @param defaultValue Value to return if key not found
     * @return Boolean value
     *
     * @details
     * Accepts (case-insensitive):
     * - true: "true", "1", "yes", "on"
     * - false: "false", "0", "no", "off"
     *
     * @example
     * @code
     * bool debug = config.getBool("debug_mode", false);
     * bool useSSL = config.getBool("use_ssl", true);
     * @endcode
     */
    bool getBool(const std::string& key, bool defaultValue = false) const;

    /**
     * @brief Get double/float value from config
     * @param key Configuration key
     * @param defaultValue Value to return if key not found
     * @return Double value
     *
     * @example
     * @code
     * double timeout = config.getDouble("timeout_seconds", 5.0);
     * double frameRate = config.getDouble("frame_rate", 24.0);
     * @endcode
     */
    double getDouble(const std::string& key, double defaultValue = 0.0) const;

    // ==================== UTILITY METHODS ====================

    /**
     * @brief Check if key exists in configuration
     * @param key Configuration key to check
     * @return true if key exists, false otherwise
     *
     * @example
     * @code
     * if (config.hasKey("custom_setting")) {
     *     // Use custom setting
     * } else {
     *     // Use default behavior
     * }
     * @endcode
     */
    bool hasKey(const std::string& key) const;

    /**
     * @brief Get config file path
     * @return Path to configuration file used
     */
    std::string getConfigFilePath() const { return configFilePath_; }

    /**
     * @brief Print all configuration values to console
     * @note Useful for debugging configuration issues
     */
    void dump() const;
};

#endif