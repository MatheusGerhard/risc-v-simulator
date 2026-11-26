#include "AluPanel.hpp"

#include <imgui.h>

#include <bit>

namespace rv32i {

void drawAluPanel(const Estado& state) {
    ImGui::Text("Operando A: %d  (0x%08X)", state.latA,
                std::bit_cast<std::uint32_t>(state.latA));
    ImGui::Text("ALU (Z/N): %s / %s", state.aluZ ? "1" : "0", state.aluN ? "1" : "0");
    ImGui::Text("Saída ALU: %d  (0x%08X)", state.aluRes,
                std::bit_cast<std::uint32_t>(state.aluRes));
}

} // namespace rv32i
