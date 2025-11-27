#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <memory>
#include <mutex>
#include <stdexcept>

/**
 * @file Singleton.hpp
 * @brief Singleton Design Pattern - Đảm bảo một class chỉ có duy nhất một instance
 * 
 * @details
 * Singleton pattern đảm bảo một class chỉ có duy nhất một instance trong toàn bộ chương trình
 * và cung cấp một điểm truy cập toàn cục (global access point) đến instance đó.
 * 
 * Trong dự án này, Singleton được sử dụng cho:
 * - **Logger**: Chỉ có một hệ thống logging duy nhất trong toàn bộ app
 * - **Config**: Một cấu hình duy nhất được load từ file và sử dụng ở mọi nơi
 * - **Metrics**: Một nơi thu thập thống kê performance cho toàn bộ hệ thống
 * 
 * Implementation này thread-safe sử dụng:
 * - Meyer's Singleton (C++11 static local variable - thread-safe by standard)
 * - std::call_once cho double-checked locking (nếu cần lazy initialization với params)
 * 
 * @example Sử dụng Meyer's Singleton (Simple):
 * @code
 * class Logger : public Singleton<Logger> {
 *     friend class Singleton<Logger>;  // Allow Singleton to access private constructor
 * 
 * private:
 *     Logger() { 
 *         // Private constructor - ngăn tạo instance từ bên ngoài
 *         std::cout << "Logger initialized" << std::endl;
 *     }
 * 
 * public:
 *     void log(const std::string& msg) {
 *         std::cout << "[LOG] " << msg << std::endl;
 *     }
 *     
 *     void error(const std::string& msg) {
 *         std::cerr << "[ERROR] " << msg << std::endl;
 *     }
 * };
 * 
 * // Usage - đơn giản và thread-safe
 * Logger::getInstance().log("Server started");
 * Logger::getInstance().error("Connection failed");
 * 
 * // Hoặc dùng reference để gọi nhiều lần
 * auto& logger = Logger::getInstance();
 * logger.log("Processing request");
 * logger.log("Request completed");`
 * @endcode
 * 
 * @example Sử dụng SingletonWithInit (Với tham số khởi tạo):
 * @code
 * // Singleton cần tham số initialization
 * class Config : public SingletonWithInit<Config> {
 *     friend class SingletonWithInit<Config>;
 * 
 * private:
 *     std::string configFile_;
 *     int port_;
 *     
 *     Config(const std::string& file) : configFile_(file) {
 *         // Load config từ file
 *         port_ = 8554;  // Default
 *     }
 * 
 * public:
 *     int getPort() const { return port_; }
 *     const std::string& getConfigFile() const { return configFile_; }
 *     
 *     static void initialize(const std::string& configFile) {
 *         std::call_once(initFlag_, [&]() {
 *             instance_.reset(new Config(configFile));
 *         });
 *     }
 * };
 * 
 * // Usage
 * Config::initialize("server.conf");  // Khởi tạo với tham số
 * 
 * int port = Config::getInstance().getPort();
 * std::string file = Config::getInstance().getConfigFile();
 * @endcode
 * 
 * @warning Lưu ý quan trọng:
 * - Constructor của derived class PHẢI là private/protected
 * - Phải khai báo `friend class Singleton<DerivedClass>` để Singleton có thể gọi constructor
 * - Copy constructor và assignment operator bị xóa (non-copyable)
 * - Move constructor và assignment operator bị xóa (non-movable)
 * - Destruction order của multiple singletons có thể gây vấn đề nếu có dependencies
 * 
 * @note Thread Safety:
 * - Meyer's Singleton (C++11+) thread-safe by default (magic statics)
 * - SingletonWithInit dùng std::call_once để đảm bảo thread-safe initialization
 */

/**
 * @class Singleton
 * @brief Meyer's Singleton - Thread-safe, lazy initialization, không cần parameters
 * 
 * @details
 * Implementation sử dụng C++11 "magic statics":
 * - Static local variable trong function được khởi tạo thread-safe
 * - Lazy initialization: chỉ tạo khi lần đầu tiên gọi getInstance()
 * - Automatic destruction khi program kết thúc
 * - Không cần mutex hay std::call_once
 * 
 * @tparam T Type của singleton class (CRTP pattern)
 * 
 * @note Đây là implementation đơn giản nhất và được khuyến nghị sử dụng
 * @note Nếu cần truyền parameters vào constructor, dùng SingletonWithInit
 */
template <typename T>
class Singleton
{
protected:
    /**
     * @brief Protected constructor - chỉ derived class có thể gọi
     * @note Prevent direct instantiation from outside
     */
    Singleton() = default;

    /**
     * @brief Protected destructor
     */
    virtual ~Singleton() = default;

public:
    // Delete copy constructor và assignment operator
    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;

    // Delete move constructor và move assignment
    Singleton(Singleton &&) = delete;
    Singleton &operator=(Singleton &&) = delete;

    /**
     * @brief Lấy instance duy nhất của singleton
     * @return Reference đến singleton instance
     *
     * @details
     * Meyer's Singleton implementation:
     * - C++11 đảm bảo static local variable được khởi tạo thread-safe
     * - Lazy initialization: chỉ tạo khi lần đầu tiên gọi getInstance()
     * - Automatic destruction khi program kết thúc
     *
     * @note Thread-safe từ C++11 trở đi (magic statics)
     */
    static T &getInstance()
    {
        static T instance; // Guaranteed to be thread-safe since C++11
        return instance;
    }
};

/**
 * @class SingletonWithInit
 * @brief Singleton variant với explicit initialization và constructor parameters
 * 
 * @details
 * Sử dụng khi cần:
 * - Truyền tham số vào constructor (config file path, connection params, etc.)
 * - Control chính xác thời điểm initialization (trước khi sử dụng)
 * - Lazy initialization với thread-safety guarantee
 * - Có thể destroy và recreate instance (useful cho testing)
 * 
 * Khác biệt với Singleton thông thường:
 * - Cần gọi initialize() trước khi dùng getInstance()
 * - Dùng std::unique_ptr để quản lý instance
 * - Dùng std::call_once để đảm bảo thread-safe initialization
 * - Có thể destroy() instance (Singleton thông thường không thể)
 * 
 * @tparam T Type của singleton class (CRTP pattern)
 * 
 * @note Phải gọi initialize() trước getInstance(), nếu không sẽ throw exception
 * @note Derived class phải implement static initialize() method
 * 
 * @warning initialize() chỉ có thể gọi một lần duy nhất (protected by std::call_once)
 */
template <typename T>
class SingletonWithInit
{
protected:
    static std::unique_ptr<T> instance_;
    static std::once_flag initFlag_;

    SingletonWithInit() = default;
    virtual ~SingletonWithInit() = default;

public:
    SingletonWithInit(const SingletonWithInit &) = delete;
    SingletonWithInit &operator=(const SingletonWithInit &) = delete;
    SingletonWithInit(SingletonWithInit &&) = delete;
    SingletonWithInit &operator=(SingletonWithInit &&) = delete;

    /**
     * @brief Lấy instance (phải được initialize trước)
     * @return Reference đến singleton instance
     * @throws std::runtime_error nếu chưa được initialize
     */
    static T &getInstance()
    {
        if (!instance_)
        {
            throw std::runtime_error("Singleton not initialized. Call initialize() first.");
        }
        return *instance_;
    }

    /**
     * @brief Kiểm tra xem singleton đã được initialize chưa
     * @return true nếu đã initialize
     */
    static bool isInitialized()
    {
        return instance_ != nullptr;
    }

    /**
     * @brief Destroy singleton instance (optional, for testing)
     */
    static void destroy()
    {
        instance_.reset();
    }
};

// Static member initialization
template <typename T>
std::unique_ptr<T> SingletonWithInit<T>::instance_ = nullptr;

template <typename T>
std::once_flag SingletonWithInit<T>::initFlag_;

#endif // SINGLETON_HPP
