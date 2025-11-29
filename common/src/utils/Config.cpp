#include "utils/Config.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "utils/Logger.hpp"

Config::Config(const std::string& configFile)
    : configMap_(), configFilePath_(configFile), mutex_() {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ifstream ifs(configFile);
    if (!ifs) {
        // File not found or cannot open: set sensible defaults
        configFilePath_ = configFile;
        // Defaults as documented in header
        configMap_["server_port"] = "8554";
        configMap_["rtp_port_start"] = "25000";
        configMap_["rtp_port_end"] = "26000";
        configMap_["video_file"] = "movie.Mjpeg";
        configMap_["max_clients"] = "10";
        configMap_["frame_rate"] = "24";
        configMap_["buffer_size"] = "2048";
        configMap_["log_level"] = "INFO";
        configMap_["log_file"] = "server.log";
        configMap_["debug_mode"] = "false";
        return;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        // Trim whitespace helpers
        auto ltrim = [](std::string& s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                            [](unsigned char ch) { return !std::isspace(ch); }));
        };
        auto rtrim = [](std::string& s) {
            s.erase(std::find_if(s.rbegin(), s.rend(),
                                 [](unsigned char ch) { return !std::isspace(ch); })
                        .base(),
                    s.end());
        };

        ltrim(line);
        rtrim(line);

        if (line.empty() || line[0] == '#')
            continue;

        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;  // skip malformed lines

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        ltrim(key);
        rtrim(key);
        ltrim(value);
        rtrim(value);

        if (!key.empty())
            configMap_[key] = value;
    }

    // Fill missing defaults
    if (configMap_.find("server_port") == configMap_.end())
        configMap_["server_port"] = "8554";
    if (configMap_.find("video_file") == configMap_.end())
        configMap_["video_file"] = "movie.Mjpeg";
    if (configMap_.find("debug_mode") == configMap_.end())
        configMap_["debug_mode"] = "false";
    if (configMap_.find("log_level") == configMap_.end())
        configMap_["log_level"] = "INFO";
    if (configMap_.find("log_file") == configMap_.end())
        configMap_["log_file"] = "server.log";
}

bool Config::loadFromFile(const std::string& filename) {
    std::ifstream ifs(filename);
    if (!ifs)
        return false;

    std::map<std::string, std::string> tmp;
    std::string line;

    auto ltrim_fn = [](std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                        [](unsigned char ch) { return !std::isspace(ch); }));
    };
    auto rtrim_fn = [](std::string& s) {
        s.erase(
            std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
    };

    while (std::getline(ifs, line)) {
        ltrim_fn(line);
        rtrim_fn(line);
        if (line.empty() || line[0] == '#')
            continue;

        auto pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        ltrim_fn(key);
        rtrim_fn(key);
        ltrim_fn(value);
        rtrim_fn(value);

        if (!key.empty())
            tmp[key] = value;
    }

    // Commit parsed values into configMap_
    std::lock_guard<std::mutex> lock(mutex_);
    configMap_.swap(tmp);
    configFilePath_ = filename;

    return true;
}

void Config::setDefaults() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (configMap_.find("server_port") == configMap_.end())
        configMap_["server_port"] = "8554";
    if (configMap_.find("rtp_port_start") == configMap_.end())
        configMap_["rtp_port_start"] = "25000";
    if (configMap_.find("rtp_port_end") == configMap_.end())
        configMap_["rtp_port_end"] = "26000";
    if (configMap_.find("video_file") == configMap_.end())
        configMap_["video_file"] = "movie.Mjpeg";
    if (configMap_.find("max_clients") == configMap_.end())
        configMap_["max_clients"] = "10";
    if (configMap_.find("frame_rate") == configMap_.end())
        configMap_["frame_rate"] = "24";
    if (configMap_.find("buffer_size") == configMap_.end())
        configMap_["buffer_size"] = "2048";
    if (configMap_.find("log_level") == configMap_.end())
        configMap_["log_level"] = "INFO";
    if (configMap_.find("log_file") == configMap_.end())
        configMap_["log_file"] = "server.log";
    if (configMap_.find("debug_mode") == configMap_.end())
        configMap_["debug_mode"] = "false";
}

void Config::printConfig() const {
    // Copy snapshot under lock to minimize time holding mutex
    std::map<std::string, std::string> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = configMap_;
    }

    // Print in file-like format: comment line then key=value lines
    std::cout << "# Configuration (" << configFilePath_ << ")\n";
    for (const auto& p : snapshot) {
        std::cout << p.first << "=" << p.second << '\n';
    }
}

int Config::getInt(const std::string& key, int defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configMap_.find(key);
    if (it == configMap_.end())
        return defaultValue;
    try {
        return std::stoi(it->second);
    } catch (...) {
        return defaultValue;
    }
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configMap_.find(key);
    if (it == configMap_.end())
        return defaultValue;
    return it->second;
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configMap_.find(key);
    if (it == configMap_.end())
        return defaultValue;

    std::string v = it->second;
    // to lower
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return std::tolower(c); });
    if (v == "true" || v == "1" || v == "yes" || v == "on")
        return true;
    if (v == "false" || v == "0" || v == "no" || v == "off")
        return false;
    return defaultValue;
}

double Config::getDouble(const std::string& key, double defaultValue) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configMap_.find(key);
    if (it == configMap_.end())
        return defaultValue;
    try {
        return std::stod(it->second);
    } catch (...) {
        return defaultValue;
    }
}

bool Config::hasKey(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return configMap_.find(key) != configMap_.end();
}

void Config::dump() const {
    printConfig();
}