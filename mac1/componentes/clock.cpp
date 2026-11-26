#include "clock.hpp"

#include "../controlUnit.hpp"

#include <cmath>

namespace rv32i {

void Clock::setVelocidade(double milliseconds) {
    intervaloMs = std::isfinite(milliseconds) && milliseconds > 0.0 ? milliseconds : 1000.0;
    if (emExecucao && currentControlUnit_) {
        pausar();
        iniciar(*currentControlUnit_);
    }
}

void Clock::subPulso(ControlUnit& controlUnit) {
    currentControlUnit_ = &controlUnit;
    if (halt) return;
    if (!controlUnit.rodarCiclo(subciclo, totalCiclos)) {
        pausar();
        return;
    }
    ++subciclo;
    if (subciclo > 5) {
        subciclo = 1;
        ++totalCiclos;
        if (onCicloChange) onCicloChange(totalCiclos);
    }
}

void Clock::pulso(ControlUnit& controlUnit) {
    for (int i = 0; i < 5; ++i) subPulso(controlUnit);
    if (onCicloChange) onCicloChange(totalCiclos);
}

void Clock::iniciar(ControlUnit& controlUnit, TimePoint now) {
    if (emExecucao) return;
    currentControlUnit_ = &controlUnit;
    emExecucao = true;
    lastUpdate_ = now;
    accumulator_ = std::chrono::duration<double, std::milli>::zero();
}

void Clock::pausar() noexcept { emExecucao = false; }

std::size_t Clock::atualizar(ControlUnit& controlUnit, TimePoint now) {
    currentControlUnit_ = &controlUnit;
    if (!emExecucao) return 0;
    if (lastUpdate_ == TimePoint{}) lastUpdate_ = now;
    accumulator_ += now - lastUpdate_;
    lastUpdate_ = now;
    constexpr std::size_t maxSubcyclesPerFrame = 100;
    std::size_t steps = 0;
    const std::chrono::duration<double, std::milli> interval{intervaloMs};
    while (accumulator_ >= interval && steps < maxSubcyclesPerFrame && emExecucao) {
        accumulator_ -= interval;
        subPulso(controlUnit);
        ++steps;
    }
    if (steps == maxSubcyclesPerFrame && accumulator_ >= interval) {
        accumulator_ = std::chrono::duration<double, std::milli>{
            std::fmod(accumulator_.count(), interval.count())};
    }
    return steps;
}

std::uint64_t Clock::getCycles() const noexcept { return totalCiclos; }
std::uint8_t Clock::getSC() const noexcept { return subciclo; }

void Clock::reset() {
    pausar();
    totalCiclos = 0;
    subciclo = 1;
    accumulator_ = std::chrono::duration<double, std::milli>::zero();
    if (onCicloChange) onCicloChange(totalCiclos);
}

void Clock::resetClock() { reset(); }

} // namespace rv32i
