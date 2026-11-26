#include "MemoryPanel.hpp"

#include "BinaryUploader/BinaryUploader.hpp"
#include "memoryDisplay/MemoryDisplay.hpp"
#include "../../simulador.hpp"

#include <imgui.h>

namespace rv32i {

void drawMemoryPanel(MacContext& context, Clock::TimePoint now, Simulador& simulator) {
    auto& machine = context.selected();
    ImGui::TextUnformatted("Memória e clock");
    ImGui::Separator();
    if (ImGui::Button("Clear", ImVec2(62, 0))) {
        simulator.setTurboMode(false);
        machine.clock.resetClock();
        machine.memory.clearMemory();
        machine.uc.hardReset(0);
        context.statusMessage = "Memória limpa e processador reiniciado.";
        context.ipsMeter.reset();
    }
    ImGui::SameLine();
    const bool start = ImGui::Button("##start", ImVec2(40, 0));
    {
        const auto min = ImGui::GetItemRectMin();
        const auto max = ImGui::GetItemRectMax();
        const float midY = (min.y + max.y) * 0.5F;
        const float midX = (min.x + max.x) * 0.5F;
        ImGui::GetWindowDrawList()->AddTriangleFilled(
            ImVec2(midX - 5.0F, midY - 8.0F), ImVec2(midX - 5.0F, midY + 8.0F),
            ImVec2(midX + 7.0F, midY), ImGui::GetColorU32(ImGuiCol_Text));
    }
    if (start) {
        simulator.setTurboMode(false);
        machine.clock.setVelocidade(context.clockIntervalMs);
        machine.clock.iniciar(machine.uc, now);
    }
    ImGui::SameLine();
    const bool pause = ImGui::Button("##pause", ImVec2(40, 0));
    {
        const auto min = ImGui::GetItemRectMin();
        const auto max = ImGui::GetItemRectMax();
        const float midY = (min.y + max.y) * 0.5F;
        const float midX = (min.x + max.x) * 0.5F;
        const auto color = ImGui::GetColorU32(ImGuiCol_Text);
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(midX - 6.0F, midY - 7.0F),
                                                   ImVec2(midX - 2.0F, midY + 7.0F), color);
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(midX + 2.0F, midY - 7.0F),
                                                   ImVec2(midX + 6.0F, midY + 7.0F), color);
    }
    if (pause) {
        simulator.setTurboMode(false);
        machine.clock.pausar();
    }
    if (!context.turboMode)
        context.ipsMeter.update(machine.uc.getCompletedInstructions(), machine.clock.emExecucao, now);
    ImGui::SameLine();
    if (ImGui::Button(context.turboMode ? "Turbo ON" : "Modo Turbo"))
        simulator.setTurboMode(!context.turboMode);

    ImGui::SetNextItemWidth(100.0F);
    ImGui::InputDouble("Tempo (ms)", &context.clockIntervalMs, 1.0, 10.0, "%.1f");
    ImGui::SameLine();
    if (ImGui::Button("Set Clock")) machine.clock.setVelocidade(context.clockIntervalMs);
    ImGui::Text("Ciclo %llu · SC %u · %.1f ms",
                static_cast<unsigned long long>(machine.clock.getCycles()),
                static_cast<unsigned>(machine.clock.getSC()), machine.clock.intervaloMs);
    ImGui::Text("IPS Real: %s", context.ipsMeter.formatted(machine.clock.emExecucao || context.turboMode).c_str());

    drawBinaryUploader(context);
    if (ImGui::Button("+ Memória")) {
        for (auto& view : context.memoryViews) {
            if (!view.open) {
                view.open = true;
                break;
            }
        }
    }
    ImGui::SameLine();
    int openViews = 0;
    for (const auto& view : context.memoryViews) openViews += view.open ? 1 : 0;
    ImGui::Text("%d/3 visualizações", openViews);
    drawMemoryDisplays(context);
}

} // namespace rv32i
