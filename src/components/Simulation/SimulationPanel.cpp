#include "SimulationPanel.hpp"

#include "../ALU/AluPanel.hpp"

#include <imgui.h>

#include <bit>
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <string>

namespace rv32i {
namespace {
void valueLine(const char* label, std::uint32_t value) {
    ImGui::Text("%s: 0x%08X  (%u)", label, value, value);
}

const char* registerModeName(RegisterDisplayMode mode) {
    switch (mode) {
    case RegisterDisplayMode::Hex: return "HEX";
    case RegisterDisplayMode::Dec: return "DEC";
    case RegisterDisplayMode::Bin: return "BIN";
    }
    return "HEX";
}

void advanceRegisterMode(RegisterDisplayMode& mode) {
    switch (mode) {
    case RegisterDisplayMode::Hex: mode = RegisterDisplayMode::Dec; break;
    case RegisterDisplayMode::Dec: mode = RegisterDisplayMode::Bin; break;
    case RegisterDisplayMode::Bin: mode = RegisterDisplayMode::Hex; break;
    }
}

std::string registerValue(const RegisterInfo& reg, RegisterDisplayMode mode) {
    if (mode == RegisterDisplayMode::Dec) return std::to_string(reg.dec);
    if (mode == RegisterDisplayMode::Bin)
        return std::bitset<16>(static_cast<std::uint16_t>(reg.dec)).to_string();
    return reg.hex;
}
}

void drawSimulationPanel(MacContext& context) {
    static constexpr const char* processors[]{"RV32I (Datapath)", "RV32I (Cache D)", "RV32I (Cache A)"};
    auto selected = static_cast<int>(context.macAtivo);
    if (ImGui::Combo("Processador", &selected, processors, IM_ARRAYSIZE(processors)))
        context.macAtivo = static_cast<MacAtivo>(selected);
    ImGui::TextUnformatted("Tamanho da cache:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(130.0F);
    ImGui::InputInt("##cache-size", &context.cacheSize, 1, 4);
    context.cacheSize = context.cacheSize < 1 ? 1 : context.cacheSize;
    for (auto& lane : context.units) {
        if (lane.cache && lane.cache->size() != static_cast<std::size_t>(context.cacheSize))
            lane.cache->resize(static_cast<std::size_t>(context.cacheSize));
    }

    auto& machine = context.selected();
    const auto index = static_cast<std::size_t>(context.macAtivo);
    const auto state = context.stateSnapshot
        ? (*context.stateSnapshot)[index]
        : machine.uc.getEstado(machine.clock.getSC(), machine.clock.getCycles());
    ImGui::SeparatorText("Registradores x0–x31");
    ImGui::Text("Base: %s", registerModeName(context.registerDisplayMode));
    ImGui::SameLine();
    if (ImGui::SmallButton("Alternar base")) advanceRegisterMode(context.registerDisplayMode);
    if (ImGui::BeginTable("RegisterTable", 3,
                          ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV |
                              ImGuiTableFlags_SizingStretchSame,
                          ImVec2(0, 190))) {
        ImGui::TableSetupColumn("Reg.");
        ImGui::TableSetupColumn("ABI");
        ImGui::TableSetupColumn(registerModeName(context.registerDisplayMode));
        ImGui::TableHeadersRow();
        for (const auto& reg : state.registersList) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("x%u", reg.index);
            ImGui::TableNextColumn(); ImGui::TextUnformatted(reg.abi.c_str());
            ImGui::TableNextColumn(); {
                const auto value = registerValue(reg, context.registerDisplayMode);
                ImGui::TextUnformatted(value.c_str());
            }
        }
        ImGui::EndTable();
    }
    ImGui::SeparatorText("Clock & FSM");
    ImGui::Text("Etapa: %s", state.stage.c_str());
    ImGui::Text("Mnemônico: %s", state.decoded.mnemonic.c_str());
    ImGui::Text("Formato/Imm: %s / %d", state.decoded.format.c_str(), state.decoded.imm);
    valueLine("PC", state.pc);
    ImGui::Text("Desvio: %s  ·  Jump: %s", state.branchTaken ? "tomado" : "não tomado",
                state.jumpTaken ? "tomado" : "não tomado");
    std::uint32_t nextPc = state.pc + 4U;
    if (state.stage == "WRITEBACK") nextPc = state.pc;
    else if (state.branchTaken || (state.jumpTaken && state.decoded.opcode == 0x6f))
        nextPc = state.pc + static_cast<std::uint32_t>(state.decoded.imm);
    else if (state.jumpTaken && state.decoded.opcode == 0x67)
        nextPc = (state.registers[state.decoded.rs1] + static_cast<std::uint32_t>(state.decoded.imm)) & ~1U;
    valueLine("Próx. PC", nextPc);
    valueLine("MAR", state.mar);
    valueLine("MBR/MDR", state.mbr);
    ImGui::Text("RS1: x%u  ·  RS2: x%u", state.decoded.rs1, state.decoded.rs2);
    ImGui::Text("Operando B: %d", state.latB);
    drawAluPanel(state);
    ImGui::SeparatorText("Conteúdo da Cache");
    if (machine.cache == nullptr) {
        ImGui::TextDisabled("Modo Direto");
    } else if (ImGui::BeginTable("DataCacheTable", 4,
                                 ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                     ImGuiTableFlags_SizingStretchProp,
                                 ImVec2(0.0F, 170.0F))) {
        ImGui::TableSetupColumn("Linha", ImGuiTableColumnFlags_WidthFixed, 48.0F);
        ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_WidthFixed, 32.0F);
        ImGui::TableSetupColumn("Endereço", ImGuiTableColumnFlags_WidthFixed, 205.0F);
        ImGui::TableSetupColumn("Palavras");
        ImGui::TableHeadersRow();
        for (const auto& line : machine.cache->getSnapshot()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%zu", line.index);
            ImGui::TableNextColumn(); ImGui::Text("%d", line.valid ? 1 : 0);
            ImGui::TableNextColumn();
            if (line.hasContent)
                ImGui::Text("0x%08X-0x%08X", line.address, line.address + 15U);
            else ImGui::TextDisabled("-");
            ImGui::TableNextColumn();
            if (line.hasContent) {
                ImGui::Text("%08X %08X %08X %08X", line.words[0], line.words[1],
                            line.words[2], line.words[3]);
            } else ImGui::TextDisabled("-");
        }
        ImGui::EndTable();
    }
}

} // namespace rv32i
