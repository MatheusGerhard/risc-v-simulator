#include "InstructionsPanel.hpp"

#include "../../../montador/montador.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <cfloat>
#include <string_view>

namespace rv32i {
namespace {
std::string problematicLine(std::string_view message) {
    constexpr std::string_view prefix = "Erro na instrução '";
    constexpr std::string_view suffix = "' (PC ";
    const auto start = message.find(prefix);
    if (start == std::string_view::npos) return {};
    const auto lineStart = start + prefix.size();
    const auto lineEnd = message.find(suffix, lineStart);
    if (lineEnd == std::string_view::npos) return {};
    return std::string(message.substr(lineStart, lineEnd - lineStart));
}
}

void drawInstructionsPanel(MacContext& context) {
    if (ImGui::CollapsingHeader("Instruções", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextWrapped("Edite o programa Assembly e monte-o na memória do processador selecionado.");
        ImGui::InputTextMultiline("##assembly", &context.assemblyCode, ImVec2(-FLT_MIN, 290.0F));
        if (ImGui::Button("Montar Programa RV32I", ImVec2(-FLT_MIN, 0))) {
            Montador assembler(context.assemblyCode);
            if (const auto error = assembler.main()) {
                context.assemblyError = *error;
                context.problematicInstruction = problematicLine(*error);
                context.assemblyErrorOpen = true;
                ImGui::OpenPopup("Erro de montagem");
            } else {
                auto& machine = context.selected();
                machine.clock.resetClock();
                machine.memory.clearMemory();
                assembler.preencherMemoria(machine.memory, SimulationLog::instance());
                machine.uc.hardReset(0);
                context.ipsMeter.reset();
                context.statusMessage = "Programa montado na memória. PC reiniciado para 0.";
            }
        }
    }
    if (!context.statusMessage.empty()) {
        ImGui::Spacing();
        ImGui::TextWrapped("%s", context.statusMessage.c_str());
    }
    ImGui::SetNextWindowSize(ImVec2(480.0F, 220.0F), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Erro de montagem", &context.assemblyErrorOpen,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0F, 0.36F, 0.36F, 1.0F), "O montador encontrou um erro:");
        ImGui::Spacing();
        if (!context.problematicInstruction.empty()) {
            ImGui::TextUnformatted("Instrução problemática:");
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0F, 0.80F, 0.45F, 1.0F));
            ImGui::TextWrapped("%s", context.problematicInstruction.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }
        ImGui::TextWrapped("%s", context.assemblyError.c_str());
        ImGui::Spacing();
        if (ImGui::Button("Fechar", ImVec2(120.0F, 0.0F))) {
            context.assemblyErrorOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

} // namespace rv32i
