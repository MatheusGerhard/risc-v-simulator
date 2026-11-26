#include "ProcessorTrack.hpp"

#include <algorithm>
#include <string>

namespace rv32i {

void drawProcessorTrack(const ProcessorPerformance& performance, ImVec4 accent,
                        std::size_t programSize) {
    ImGui::PushStyleColor(ImGuiCol_Border, accent);
    ImGui::BeginChild(PerformanceTracker::processorName(performance.processor),
                      ImVec2(0.0F, 0.0F), ImGuiChildFlags_Borders);
    ImGui::TextColored(accent, "%s", PerformanceTracker::processorName(performance.processor));
    const auto& current = performance.currentInstruction;
    ImGui::TextWrapped("%s", current ? current->instructionText.c_str() : "Aguardando");
    const float progress = programSize == 0 ? 0.0F : std::clamp(
        static_cast<float>(performance.completedInstructions) / static_cast<float>(programSize),
        0.0F, 1.0F);
    ImGui::ProgressBar(progress, ImVec2(-FLT_MIN, 0.0F), "Progresso");
    ImGui::Text("Concluidas: %llu  |  Total: %llu ciclos",
                static_cast<unsigned long long>(performance.completedInstructions),
                static_cast<unsigned long long>(performance.totalCycles));
    const double cpi = performance.completedInstructions == 0 ? 0.0 :
        static_cast<double>(performance.totalCycles) / static_cast<double>(performance.completedInstructions);
    ImGui::Text("CPI: %.2f  |  IPC: %.2f", cpi, performance.ipc);
    ImGui::Text("Penalidade de cache: %llu ciclos",
                static_cast<unsigned long long>(performance.cyclePenaltyCycles));
    const auto accesses = performance.cacheHits + performance.cacheMisses;
    const auto hitPercent = performance.cacheHitRate
        ? static_cast<int>(*performance.cacheHitRate * 100.0 + 0.5) : 0;
    ImGui::Text("Cache: %llu hits / %llu misses (%d%%)",
                static_cast<unsigned long long>(performance.cacheHits),
                static_cast<unsigned long long>(performance.cacheMisses),
                accesses == 0 ? 0 : hitPercent);
    ImGui::SeparatorText("Instrucoes recentes");
    ImGui::BeginChild("instruction-history", ImVec2(0.0F, -FLT_MIN), ImGuiChildFlags_Borders);
    if (performance.instructions.empty()) ImGui::TextDisabled("Sem execucao");
    for (const auto& instruction : performance.instructions) {
        ImGui::Text("%s  [%llu ciclo(s)]", instruction.instructionText.c_str(),
                    static_cast<unsigned long long>(instruction.cycles));
    }
    ImGui::EndChild();
    ImGui::EndChild();
    ImGui::PopStyleColor();
}

} // namespace rv32i
