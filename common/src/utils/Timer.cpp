#include "utils/Timer.hpp"

Timer::Timer(int intervalMs) : intervalMs_(intervalMs) {
    // Khởi tạo thời gian bắt đầu ngay lập tức để tránh giá trị rác
    startTime_ = std::chrono::steady_clock::now();
    lastWaitTime_ = startTime_;
}

void Timer::start() {
    startTime_ = std::chrono::steady_clock::now();
}

// Tính toán thời gian còn thiếu và ngủ bù để đạt đúng FPS mục tiêu
void Timer::wait() {
    // Lấy thời điểm hiện tại (sau khi đã xử lý xong công việc)
    auto now = std::chrono::steady_clock::now();

    // Tính thời gian đã trôi qua kể từ lúc gọi start()
    //  duration_cast chuyển đổi khoảng thời gian về đơn vị mili-giây
    auto elapsedDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_);
    int elapsedMs = static_cast<int>(elapsedDuration.count());

    // Tính thời gian cần ngủ = Thời gian mục tiêu - Thời gian đã làm việc
    int sleepTime = intervalMs_ - elapsedMs;

    // 4. Logic ngủ bù
    if (sleepTime > 0) {
        // Nếu làm việc xong sớm, ngủ nốt phần thời gian còn lại
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
    }
    // Ngược lại (sleepTime <= 0):
    // Có nghĩa là việc xử lý (đọc/gửi) tốn nhiều thời gian hơn quy định (bị trễ).
    // Ta KHÔNG ngủ, mà return ngay lập tức để cố gắng đuổi kịp tiến độ cho frame sau.

    // Cập nhật thời điểm kết thúc wait (để dùng cho các thuật toán phức tạp hơn nếu cần)
    lastWaitTime_ = std::chrono::steady_clock::now();
}

double Timer::getElapsed() const {
    auto now = std::chrono::steady_clock::now();
    // Trả về số thực (double) để có độ chính xác cao hơn
    std::chrono::duration<double, std::milli> elapsed = now - startTime_;
    return elapsed.count();
}

// Thay đổi FPS khi đang chạy (Ví dụ: Client yêu cầu tua nhanh)
void Timer::setInterval(int intervalMs) {
    if (intervalMs > 0) {
        intervalMs_ = intervalMs;
    }
}
