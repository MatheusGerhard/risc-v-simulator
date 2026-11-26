#include "MemoryDisplay.hpp"

#include "../BinaryUploader/BinaryUploader.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <bit>
#include <bitset>
#include <cstdint>
#include <string>

namespace rv32i {
namespace {
const char* modeName(MemoryDisplayMode mode) {
    switch (mode) {
    case MemoryDisplayMode::Hex: return "HEX";
    case MemoryDisplayMode::Bin: return "BIN";
    case MemoryDisplayMode::Dec: return "DEC";
    }
    return "HEX";
}

void advanceMode(MemoryDisplayMode& mode) {
    switch (mode) {
    case MemoryDisplayMode::Hex: mode = MemoryDisplayMode::Bin; break;
    case MemoryDisplayMode::Bin: mode = MemoryDisplayMode::Dec; break;
    case MemoryDisplayMode::Dec: mode = MemoryDisplayMode::Hex; break;
    }
}

std::string displayValue(std::uint32_t word, MemoryDisplayMode mode) {
    if (mode == MemoryDisplayMode::Bin) return std::bitset<32>(word).to_string();
    if (mode == MemoryDisplayMode::Dec) return std::to_string(std::bit_cast<std::int32_t>(word));
    char output[11]{};
    std::snprintf(output, sizeof(output), "0x%08X", word);
    return output;
}

void drawView(MacContext& context, MemoryViewState& view, std::size_t index, float height) {
    auto& memory = context.selected().memory;
    const auto id = "MemoryView" + std::to_string(index);
    if (!ImGui::BeginChild(id.c_str(), ImVec2(0, height), ImGuiChildFlags_Borders)) {
        ImGui::EndChild();
        return;
    }
    ImGui::Text("RAM %zu", index + 1);
    ImGui::SameLine();
    if (ImGui::SmallButton(modeName(view.mode))) advanceMode(view.mode);
    ImGui::SameLine();
    if (ImGui::SmallButton("×")) view.open = false;
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 65.0F);
    ImGui::InputText(("##address" + std::to_string(index)).c_str(), &view.addressQuery,
                     ImGuiInputTextFlags_CharsNoBlank);
    ImGui::SameLine();
    if (ImGui::SmallButton(("Buscar##" + std::to_string(index)).c_str())) {
        if (const auto parsed = parseBaseAddress(view.addressQuery)) {
            const auto rows = memory.size() / 4;
            std::uint64_t line = *parsed;
            if (view.addressQuery.starts_with("0x") || view.addressQuery.starts_with("0X"))
                line /= 4;
            else if (line >= rows && line % 4 == 0)
                line /= 4;
            if (line < rows) {
                view.targetLine = static_cast<std::size_t>(line);
                view.scrollRequested = true;
            }
        }
    }

    if (view.scrollRequested) {
        ImGui::SetScrollY(static_cast<float>(view.targetLine) * ImGui::GetTextLineHeightWithSpacing());
        view.scrollRequested = false;
    }
    const auto rowCount = static_cast<int>((memory.size() + 3) / 4);
    ImGuiListClipper clipper;
    clipper.Begin(rowCount);
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            const auto address = static_cast<std::uint32_t>(row * 4);
            ImGui::Text("0x%08X: %s", address,
                        displayValue(memory.readWordUnsigned(address), view.mode).c_str());
        }
    }
    clipper.End();
    ImGui::EndChild();
}
}

void drawMemoryDisplays(MacContext& context) {
    int visible = 0;
    for (const auto& view : context.memoryViews) visible += view.open ? 1 : 0;
    if (visible == 0) {
        ImGui::TextDisabled("Nenhuma visualização de memória aberta.");
        return;
    }
    const float available = ImGui::GetContentRegionAvail().y;
    const float height = visible == 1 ? 0.0F : (available - 6.0F * (visible - 1)) / visible;
    for (std::size_t index = 0; index < context.memoryViews.size(); ++index) {
        auto& view = context.memoryViews[index];
        if (!view.open) continue;
        drawView(context, view, index, height);
    }
}

} // namespace rv32i
