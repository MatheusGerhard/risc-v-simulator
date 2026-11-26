#include "useIPS.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace rv32i {
namespace {
std::string fixed(double value, int precision) {
    std::ostringstream out;
    const double scale = precision == 1 ? 10.0 : 1.0;
    const double rounded = std::floor(value * scale + 0.5) / scale;
    out << std::fixed << std::setprecision(precision) << rounded;
    return out.str();
}
}

std::string formatIPS(double ips, bool isRunning) {
    if (!isRunning || !std::isfinite(ips) || ips <= 0.0) return "0 IPS";
    if (ips >= 1'000'000.0) return fixed(ips / 1'000'000.0, 1) + "M IPS";
    if (ips >= 1'000.0) return fixed(ips / 1'000.0, 1) + "k IPS";
    if (ips < 10.0) return fixed(ips, 1) + " IPS";
    return std::to_string(static_cast<std::uint64_t>(std::llround(ips))) + " IPS";
}

void IPSMeter::update(std::uint64_t completedInstructions, bool isRunning, TimePoint now) noexcept {
    if (!isRunning) {
        ips_ = 0.0;
        lastCount_ = completedInstructions;
        lastTime_ = now;
        initialized_ = true;
        return;
    }
    if (!initialized_ || completedInstructions < lastCount_) {
        lastCount_ = completedInstructions;
        lastTime_ = now;
        ips_ = 0.0;
        initialized_ = true;
        return;
    }
    const auto elapsed = std::chrono::duration<double>(now - lastTime_).count();
    if (elapsed < 1.0) return;
    const auto delta = completedInstructions - lastCount_;
    ips_ = delta > 0 ? static_cast<double>(delta) / elapsed : 0.0;
    lastCount_ = completedInstructions;
    lastTime_ = now;
}

void IPSMeter::reset() noexcept {
    lastTime_ = {};
    lastCount_ = 0;
    ips_ = 0.0;
    initialized_ = false;
}

double IPSMeter::ips() const noexcept { return ips_; }
std::string IPSMeter::formatted(bool isRunning) const { return formatIPS(ips_, isRunning); }

} // namespace rv32i
