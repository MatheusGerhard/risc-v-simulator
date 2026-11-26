#include "LogPanel.hpp"

#include "../../services/simulationLog.hpp"

#include <imgui.h>

namespace rv32i {
namespace {
ImVec4 labelColor(LogType type) {
    switch (type) {
    case LogType::Instruction: return ImVec4(0.35F, 0.75F, 1.0F, 1.0F);
    case LogType::MemoryWrite: return ImVec4(1.0F, 0.72F, 0.30F, 1.0F);
    case LogType::CacheHit: return ImVec4(0.35F, 0.86F, 0.52F, 1.0F);
    case LogType::CacheMiss: return ImVec4(1.0F, 0.38F, 0.38F, 1.0F);
    }
    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}
}

void drawLogPanel(MacContext& context) {
    if (!context.logOpen) return;
    ImGui::SetNextWindowSize(ImVec2(560.0F, 420.0F), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Log", &context.logOpen)) {
        ImGui::End();
        return;
    }
    if (ImGui::Button("Limpar log")) SimulationLog::instance().clear();
    ImGui::SameLine();
    const auto events = SimulationLog::instance().getEvents();
    ImGui::TextDisabled("%zu/%zu eventos · mais recentes primeiro", events.size(), SimulationLog::maxEvents);
    ImGui::Separator();
    if (ImGui::BeginChild("LogEvents", ImVec2(0, 0), ImGuiChildFlags_Borders)) {
        for (const auto& event : events) {
            ImGui::PushID(static_cast<int>(event.id & 0x7fffffffU));
            ImGui::TextColored(labelColor(event.type), "%s", event.label.c_str());
            ImGui::SameLine();
            ImGui::TextWrapped("%s", event.message.c_str());
            ImGui::Separator();
            ImGui::PopID();
        }
        if (events.empty()) ImGui::TextDisabled("Sem eventos");
    }
    ImGui::EndChild();
    ImGui::End();
}

} // namespace rv32i
