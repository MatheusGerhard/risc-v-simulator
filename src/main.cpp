#include <SDL.h>
#include <SDL_opengl.h>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl2.h>
#include <imgui.h>

#include "App.hpp"
#include "assets/CousineRegular.hpp"
#include "assets/RiscVIcon.hpp"
#include "simulador.hpp"
#include "styles/tema.hpp"

#include <cstdint>
#include <filesystem>
#include <string>

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_Window* window = SDL_CreateWindow(
        "Simulador RISC-V",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        800,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == nullptr) {
        SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_RWops* iconStream = SDL_RWFromConstMem(
        rv32i::assets::RiscVIconBmp,
        static_cast<int>(rv32i::assets::RiscVIconBmpSize));
    SDL_Surface* icon = iconStream == nullptr ? nullptr : SDL_LoadBMP_RW(iconStream, 1);
    if (icon != nullptr) {
        SDL_SetWindowIcon(window, icon);
        SDL_FreeSurface(icon);
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (gl_context == nullptr) {
        SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    rv32i::aplicarTemaEscuro();
    ImFontConfig fontConfig{};
    fontConfig.FontDataOwnedByAtlas = false;
    ImFontGlyphRangesBuilder glyphs;
    glyphs.AddRanges(ImGui::GetIO().Fonts->GetGlyphRangesDefault());
    glyphs.AddChar(0x2013); // en dash
    glyphs.AddChar(0x2016); // double vertical line, pause icon
    glyphs.AddChar(0x25BA); // play icon
    ImVector<ImWchar> glyphRanges;
    glyphs.BuildRanges(&glyphRanges);
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char*>(rv32i::assets::CousineRegularTtf),
        static_cast<int>(rv32i::assets::CousineRegularTtfSize), 15.0F, &fontConfig,
        glyphRanges.Data);
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 150");

    rv32i::Simulador simulator;
    rv32i::App app(simulator);
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event) != 0) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_DROPFILE && event.drop.file != nullptr) {
                const auto* utf8 = reinterpret_cast<const char8_t*>(event.drop.file);
                (void)simulator.carregarBinario(std::filesystem::path(std::u8string(utf8)));
                SDL_free(event.drop.file);
            }
            if (event.type == SDL_QUIT ||
                (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)) {
                running = false;
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        const auto now = rv32i::Clock::ClockSource::now();
        simulator.atualizar(now);
        app.render(now);
        ImGui::Render();

        glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x),
                   static_cast<int>(ImGui::GetIO().DisplaySize.y));
        glClearColor(0.08F, 0.08F, 0.09F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
