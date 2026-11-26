#include "App.hpp"

#include "components/Instructions/InstructionsPanel.hpp"
#include "components/Memory/MemoryPanel.hpp"
#include "components/Simulation/SimulationPanel.hpp"
#include "components/simulationLog/LogPanel.hpp"
#include "pages/RaceComparison/RaceComparison.hpp"

#include <imgui.h>

#include <mutex>

namespace rv32i {

App::App(Simulador& simulator) noexcept : simulator_(simulator) {}

void App::render(Clock::TimePoint now) {
    std::lock_guard frameLock(simulator_.stateMutex());
    auto& context = simulator_.context();
    context.stateSnapshot = simulator_.stateSnapshot();
    const auto viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr auto flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
    ImGui::Begin("RISC-V Workspace", nullptr, flags);
    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("Simulador")) {
            if (ImGui::Button(context.logOpen ? "Ocultar Log" : "Log"))
                context.logOpen = !context.logOpen;
            ImGui::Separator();
            const float available = ImGui::GetContentRegionAvail().x;
            const float left = available * 0.25F;
            const float center = available * 0.48F;
            const float right = available - left - center - 16.0F;
            if (ImGui::BeginChild("MemoryPanel", ImVec2(left, 0), ImGuiChildFlags_Borders))
                drawMemoryPanel(context, now, simulator_);
            ImGui::EndChild();
            ImGui::SameLine();
            if (ImGui::BeginChild("SimulationPanel", ImVec2(center, 0), ImGuiChildFlags_Borders))
                drawSimulationPanel(context);
            ImGui::EndChild();
            ImGui::SameLine();
            if (ImGui::BeginChild("InstructionsPanel", ImVec2(right, 0), ImGuiChildFlags_Borders))
                drawInstructionsPanel(context);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Comparação")) {
            if (ImGui::Button(context.logOpen ? "Ocultar Log" : "Log"))
                context.logOpen = !context.logOpen;
            ImGui::Separator();
            drawRaceComparison(context, simulator_);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
    drawLogPanel(context);
}

} // namespace rv32i
