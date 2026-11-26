#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace rv32i {

class IPSMeter {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    void update(std::uint64_t completedInstructions, bool isRunning,
                TimePoint now = Clock::now()) noexcept;
    void reset() noexcept;
    [[nodiscard]] double ips() const noexcept;
    [[nodiscard]] std::string formatted(bool isRunning = true) const;

private:
    TimePoint lastTime_{};
    std::uint64_t lastCount_{};
    double ips_{};
    bool initialized_{};
};

[[nodiscard]] std::string formatIPS(double ips, bool isRunning = true);

} // namespace rv32i
