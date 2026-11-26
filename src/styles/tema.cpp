#include "tema.hpp"

#include <imgui.h>

namespace rv32i {

void aplicarTemaEscuro() {
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 5.0F;
    style.ChildRounding = 5.0F;
    style.FrameRounding = 4.0F;
    style.PopupRounding = 4.0F;
    style.ScrollbarRounding = 6.0F;
    style.GrabRounding = 4.0F;
    auto& colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.118F, 0.118F, 0.118F, 1.0F); // #1e1e1e
    colors[ImGuiCol_ChildBg] = ImVec4(0.145F, 0.145F, 0.149F, 1.0F);  // #252526
    colors[ImGuiCol_PopupBg] = colors[ImGuiCol_ChildBg];
    colors[ImGuiCol_FrameBg] = ImVec4(0.20F, 0.20F, 0.21F, 1.0F);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25F, 0.30F, 0.39F, 1.0F);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.16F, 0.28F, 0.48F, 1.0F);
    colors[ImGuiCol_Button] = ImVec4(0.145F, 0.388F, 0.922F, 1.0F); // #2563eb
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.20F, 0.45F, 0.96F, 1.0F);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.12F, 0.32F, 0.78F, 1.0F);
    colors[ImGuiCol_Header] = ImVec4(0.145F, 0.388F, 0.922F, 0.42F);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.145F, 0.388F, 0.922F, 0.65F);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.145F, 0.388F, 0.922F, 0.85F);
    colors[ImGuiCol_Tab] = ImVec4(0.15F, 0.15F, 0.16F, 1.0F);
    colors[ImGuiCol_TabHovered] = colors[ImGuiCol_ButtonHovered];
    colors[ImGuiCol_TabSelected] = colors[ImGuiCol_ChildBg];
    colors[ImGuiCol_TextDisabled] = ImVec4(0.533F, 0.533F, 0.533F, 1.0F); // #888888
}

} // namespace rv32i
