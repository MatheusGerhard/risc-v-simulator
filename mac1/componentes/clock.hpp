#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace rv32i {

class ControlUnit;

class Clock {
public:
    using ClockSource = std::chrono::steady_clock;
    using TimePoint = ClockSource::time_point;

    std::uint8_t subciclo{1};
    std::uint64_t totalCiclos{};
    double intervaloMs{1000.0};
    bool emExecucao{};
    bool halt{};
    std::function<void(std::uint64_t)> onCicloChange;

    void setVelocidade(double milliseconds);
    void subPulso(ControlUnit& controlUnit);
    void pulso(ControlUnit& controlUnit);
    void iniciar(ControlUnit& controlUnit, TimePoint now = ClockSource::now());
    void pausar() noexcept;
    std::size_t atualizar(ControlUnit& controlUnit, TimePoint now = ClockSource::now());
    [[nodiscard]] std::uint64_t getCycles() const noexcept;
    [[nodiscard]] std::uint8_t getSC() const noexcept;
    void reset();
    void resetClock();

private:
    ControlUnit* currentControlUnit_{};
    TimePoint lastUpdate_{};
    std::chrono::duration<double, std::milli> accumulator_{};
};

} // namespace rv32i
