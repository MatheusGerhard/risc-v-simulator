#include "RaceComparison.hpp"

#include "ProcessorTrack.hpp"
#include "../../simulador.hpp"

#include <imgui.h>

#include <array>

namespace rv32i {

void drawRaceComparison(MacContext& context, Simulador& simulator) {
    if (ImGui::Button("Montar pistas")) (void)simulator.prepararComparacao();
    ImGui::SameLine();
    if (ImGui::Button(context.comparisonRunning ? "Pausar" : "Executar")) {
        if (!context.comparisonReady && !simulator.prepararComparacao()) {
            // Error is shown below.
        } else {
            simulator.setComparacaoEmExecucao(!context.comparisonRunning);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Próx. instrução")) {
        if (!context.comparisonReady && !simulator.prepararComparacao()) {
            // Error is shown below.
        } else {
            simulator.avancarComparacaoInstrucao();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset comparação")) simulator.resetarComparacao();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(125.0F);
    ImGui::DragScalar("Intervalo (ms)", ImGuiDataType_Double, &context.comparisonIntervalMs,
                      1.0F, nullptr, nullptr, "%.0f");
    context.comparisonIntervalMs = context.comparisonIntervalMs < 1.0 ? 1.0 : context.comparisonIntervalMs;
    ImGui::SameLine();
    ImGui::Text("%zu instruções", context.comparisonProgramSize);
    if (!context.comparisonError.empty()) {
        ImGui::TextColored(ImVec4(1.0F, 0.35F, 0.35F, 1.0F), "%s", context.comparisonError.c_str());
    }
    ImGui::Separator();

    constexpr std::array<ImVec4, 3> accents{{
        ImVec4(0.22F, 0.74F, 0.95F, 1.0F),
        ImVec4(0.25F, 0.78F, 0.47F, 1.0F),
        ImVec4(0.96F, 0.62F, 0.18F, 1.0F),
    }};
    const float trackWidth = (ImGui::GetContentRegionAvail().x - 16.0F) / 3.0F;
    const auto states = context.racePerformance.snapshot();
    for (std::size_t i = 0; i < states.size(); ++i) {
        if (i != 0) ImGui::SameLine();
        ImGui::BeginChild(("TrackColumn" + std::to_string(i)).c_str(),
                          ImVec2(trackWidth, -FLT_MIN), ImGuiChildFlags_Borders);
        drawProcessorTrack(states[i], accents[i], context.comparisonProgramSize);
        ImGui::EndChild();
    }
}

} // namespace rv32i
