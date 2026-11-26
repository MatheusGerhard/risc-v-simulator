#pragma once

#include "../../services/performanceTracker.hpp"

#include <imgui.h>

namespace rv32i {
void drawProcessorTrack(const ProcessorPerformance& performance, ImVec4 accent,
                        std::size_t programSize);
}
