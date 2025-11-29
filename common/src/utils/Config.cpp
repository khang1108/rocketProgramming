#include "utils/Config.hpp"
#include <algorithm>
#include <cctype>
<<<<<<< HEAD
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <fstream>
#include "utils/Logger.hpp"

Config::Config(const std::string &configFile)
    : configMap_(), configFilePath_(configFile), mutex_()
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::ifstream ifs(configFile);
    if (!ifs)
    {
=======
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
>>>>>>> origin/main
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
<<<<<<< HEAD
    while (std::getline(ifs, line))
    {
        // Trim whitespace helpers
        auto ltrim = [](std::string &s)
        {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                            { return !std::isspace(ch); }));
        };
        auto rtrim = [](std::string &s)
        {
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
                                 { return !std::isspace(ch); })
=======
    while (std::getline(ifs, line)) {
        // Trim whitespace helpers
        auto ltrim = [](std::string& s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(),
                                            [](unsigned char ch) { return !std::isspace(ch); }));
        };
        auto rtrim = [](std::string& s) {
            s.erase(std::find_if(s.rbegin(), s.rend(),
                                 [](unsigned char ch) { return !std::isspace(ch); })
>>>>>>> origin/main
                        .base(),
                    s.end());
        };

        ltrim(line);
        rtrim(line);

        if (line.empty() || line[0] == '#')
            continue;

        auto pos = line.find('=');
        if (pos == std::string::npos)
<<<<<<< HEAD
            continue; // skip malformed lines
=======
            continue;  // skip malformed lines
>>>>>>> origin/main

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

<<<<<<< HEAD
bool Config::loadFromFile(const std::string &filename)
{
=======
bool Config::loadFromFile(const std::string& filename) {
>>>>>>> origin/main
    std::ifstream ifs(filename);
    if (!ifs)
        return false;

    std::map<std::string, std::string> tmp;
    std::string line;

<<<<<<< HEAD
    auto ltrim_fn = [](std::string &s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                        { return !std::isspace(ch); }));
    };
    auto rtrim_fn = [](std::string &s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
                             { return !std::isspace(ch); })
                    .base(),
                s.end());
    };

    while (std::getline(ifs, line))
    {
=======
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
>>>>>>> origin/main
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

<<<<<<< HEAD
void Config::setDefaults()
{
=======
void Config::setDefaults() {
>>>>>>> origin/main
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

<<<<<<< HEAD
void Config::printConfig() const
{
=======
void Config::printConfig() const {
>>>>>>> origin/main
    // Copy snapshot under lock to minimize time holding mutex
    std::map<std::string, std::string> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = configMap_;
    }

    // Print in file-like format: comment line then key=value lines
    std::cout << "# Configuration (" << configFilePath_ << ")\n";
<<<<<<< HEAD
    for (const auto &p : snapshot)
    {
=======
    for (const auto& p : snapshot) {
>>>>>>> origin/main
        std::cout << p.first << "=" << p.second << '\n';
    }
}

<<<<<<< HEAD
int Config::getInt(const std::string &key, int defaultValue) const
{
=======
int Config::getInt(const std::string& key, int defaultValue) const {
>>>>>>> origin/main
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configMap_.find(key);
    if (it == configMap_.end())
        return defaultValue;
<<<<<<< HEAD
    try
    {
        return std::stoi(it->second);
    }
    catch (...)
    {
=======
    try {
        return std::stoi(it->second);
    } catch (...) {
>>>>>>> origin/main
        return defaultValue;
    }
}

<<<<<<< HEAD
std::string Config::getString(const std::string &key, const std::string &defaultValue) const
{
=======
std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
>>>>>>> origin/main
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configMap_.find(key);
    if (it == configMap_.end())
        return defaultValue;
    return it->second;
}

<<<<<<< HEAD
bool Config::getBool(const std::string &key, bool defaultValue) const
{
=======
bool Config::getBool(const std::string& key, bool defaultValue) const {
>>>>>>> origin/main
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configMap_.find(key);
    if (it == configMap_.end())
        return defaultValue;

    std::string v = it->second;
    // to lower
<<<<<<< HEAD
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c)
                   { return std::tolower(c); });
=======
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return std::tolower(c); });
>>>>>>> origin/main
    if (v == "true" || v == "1" || v == "yes" || v == "on")
        return true;
    if (v == "false" || v == "0" || v == "no" || v == "off")
        return false;
    return defaultValue;
}

<<<<<<< HEAD
double Config::getDouble(const std::string &key, double defaultValue) const
{
=======
double Config::getDouble(const std::string& key, double defaultValue) const {
>>>>>>> origin/main
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = configMap_.find(key);
    if (it == configMap_.end())
        return defaultValue;
<<<<<<< HEAD
    try
    {
        return std::stod(it->second);
    }
    catch (...)
    {
=======
    try {
        return std::stod(it->second);
    } catch (...) {
>>>>>>> origin/main
        return defaultValue;
    }
}

<<<<<<< HEAD
bool Config::hasKey(const std::string &key) const
{
=======
bool Config::hasKey(const std::string& key) const {
>>>>>>> origin/main
    std::lock_guard<std::mutex> lock(mutex_);
    return configMap_.find(key) != configMap_.end();
}

<<<<<<< HEAD
void Config::dump() const
{
=======
void Config::dump() const {
>>>>>>> origin/main
    printConfig();
}