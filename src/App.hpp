#pragma once

#include "simulador.hpp"

namespace rv32i {

class App {
public:
    explicit App(Simulador& simulator) noexcept;
    void render(Clock::TimePoint now);

private:
    Simulador& simulator_;
};

} // namespace rv32i
