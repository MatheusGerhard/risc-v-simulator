#pragma once

#include "../../context/MacContext.hpp"

namespace rv32i {
class Simulador;
void drawMemoryPanel(MacContext& context, Clock::TimePoint now, Simulador& simulator);
}
