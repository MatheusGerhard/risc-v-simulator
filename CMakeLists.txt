cmake_minimum_required(VERSION 3.24)

project(riscv_simulator LANGUAGES C CXX)

if(WIN32)
    enable_language(RC)
endif()

include(CTest)
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RV_BUILD_GUI "Build the SDL2 + Dear ImGui application" ON)
option(RV_BUILD_TESTS "Build the C++ test suite" ON)
set(RV_SANITIZERS_DEFAULT OFF)
if(NOT WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(RV_SANITIZERS_DEFAULT ON)
endif()
option(RV_ENABLE_SANITIZERS "Use AddressSanitizer and UndefinedBehaviorSanitizer" ${RV_SANITIZERS_DEFAULT})

# Keep the planned dependency direction in place from the start. These targets
# follow the dependency direction described in the migration plan.
add_library(rv_project_options INTERFACE)
add_library(rv_services STATIC
    src/services/simulationLog.cpp
    src/services/performanceTracker.cpp
)
target_include_directories(rv_services PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(rv_services PUBLIC rv_project_options)

add_library(rv_core STATIC
    mac1/componentes/registers.cpp
    mac1/componentes/memory.cpp
    mac1/componentes/alu.cpp
    mac1/componentes/clock.cpp
    mac1/controlUnit.cpp
    mac2/componentes/cache.cpp
    mac2/controlUnit.cpp
    mac3/controlUnit.cpp
    montador/macroinstrucoes.cpp
    montador/montador.cpp
)
target_include_directories(rv_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(rv_core PUBLIC rv_services rv_project_options)

if(MSVC)
    target_compile_options(rv_project_options INTERFACE /utf-8 /W4)
else()
    target_compile_options(rv_project_options INTERFACE -Wall -Wextra -Wpedantic)
endif()
if(RV_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(rv_project_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(rv_project_options INTERFACE -fsanitize=address,undefined)
    else()
        message(FATAL_ERROR "RV_ENABLE_SANITIZERS requires GCC or Clang")
    endif()
endif()

# Dependencies are pinned so builds don't silently change when upstream moves.
if(RV_BUILD_TESTS)
    set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
    set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.7.1.tar.gz
        URL_HASH SHA256=c991b247a1a0d7bb9c39aa35faf0fe9e19764213f28ffba3109388e62ee0269c
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(Catch2)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
        URL_HASH SHA256=0d8ef5af7f9794e3263480193c491549b2ba6cc74bb018906202ada498a79406
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()

if(RV_BUILD_GUI)
    set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "Suppress deprecated API notices from pinned dependencies" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        SDL2
        URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-2.30.11.tar.gz
        URL_HASH SHA256=cc6136dd964854e8846c679703322f3e2a341d27a06a53f8b3f642c26f1b0cfd
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(SDL2)

    FetchContent_Declare(
        imgui
        URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8-docking.tar.gz
        URL_HASH SHA256=55f5e65abea635f2a8bfa9a92cd966448a363a262cf6dead7cc662fb0ab37612
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(imgui)

    set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(NFD_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        nativefiledialog-extended
        URL https://github.com/btzy/nativefiledialog-extended/archive/refs/tags/v1.4.1.tar.gz
        URL_HASH SHA256=4a01eafa921169f009f3e83bea8e287d9e5d0b94ca9064cf7f0a546652d7c884
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nativefiledialog-extended)

    find_package(OpenGL REQUIRED)

    add_library(rv_imgui STATIC
        "${imgui_SOURCE_DIR}/imgui.cpp"
        "${imgui_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp"
        "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
    )
    target_include_directories(rv_imgui PUBLIC "${imgui_SOURCE_DIR}")
    target_link_libraries(rv_imgui PUBLIC SDL2::SDL2 OpenGL::GL)

    add_library(rv_ui STATIC
        src/App.cpp
        src/simulador.cpp
        src/styles/tema.cpp
        src/components/Memory/MemoryPanel.cpp
        src/components/Memory/BinaryUploader/BinaryUploader.cpp
        src/components/Memory/memoryDisplay/MemoryDisplay.cpp
        src/components/Simulation/SimulationPanel.cpp
        src/components/ALU/AluPanel.cpp
        src/components/Instructions/InstructionsPanel.cpp
        src/components/simulationLog/LogPanel.cpp
        src/hooks/useIPS.cpp
        src/pages/RaceComparison/ProcessorTrack.cpp
        src/pages/RaceComparison/RaceComparison.cpp
    )
    target_include_directories(rv_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(rv_ui PUBLIC rv_core rv_imgui nfd::nfd SDL2::SDL2 OpenGL::GL)
    if(WIN32)
        add_executable(rv_sim WIN32 src/main.cpp src/riscv_sim.rc)
        set_target_properties(rv_sim PROPERTIES WIN32_EXECUTABLE TRUE)
    else()
        add_executable(rv_sim src/main.cpp)
    endif()
    target_link_libraries(rv_sim PRIVATE rv_ui)
    target_compile_definitions(rv_sim PRIVATE SDL_MAIN_HANDLED)
endif()

if(RV_BUILD_TESTS)
    add_executable(rv_tests
        testes/testeFase0.cpp
        testes/testeFase1.cpp
        testes/testeAlu.cpp
        testes/testeRiscV.cpp
        testes/testeBateriaCompleta.cpp
        testes/testeMontador.cpp
        testes/testeParidade.cpp
        testes/testeBinaryLoader.cpp
        testes/testeFuzz.cpp
    )
    target_link_libraries(rv_tests PRIVATE rv_core rv_services Catch2::Catch2WithMain nlohmann_json::nlohmann_json)
    target_compile_definitions(rv_tests PRIVATE RV_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/testes/golden")
    if(RV_BUILD_GUI)
        target_sources(rv_tests PRIVATE testes/testeFase5.cpp testes/testeFase6.cpp testes/testeFase7.cpp testes/testeFase8.cpp)
        target_link_libraries(rv_tests PRIVATE rv_ui)
    endif()
    include(Catch)
    catch_discover_tests(rv_tests)
endif()

if(RV_BUILD_GUI)
    if(WIN32)
        install(TARGETS rv_sim RUNTIME_DEPENDENCY_SET rv_runtime_dependencies
            RUNTIME DESTINATION .)
    else()
        install(TARGETS rv_sim RUNTIME DESTINATION . BUNDLE DESTINATION .)
    endif()
    install(FILES README.md DESTINATION .)
    if(WIN32)
        get_filename_component(RV_COMPILER_RUNTIME_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
        install(RUNTIME_DEPENDENCY_SET rv_runtime_dependencies
            DESTINATION .
            DIRECTORIES "${RV_COMPILER_RUNTIME_DIR}"
            PRE_EXCLUDE_REGEXES
                "api-ms-win-.*" "ext-ms-win-.*"
                ".*(advapi32|combase|gdi32|glu32|imm32|kernel32|KernelBase|msvcp_win|msvcrt|ntdll|ole32|oleaut32|opengl32|rpcrt4|sechost|setupapi|shell32|user32|version|win32u|winmm).*"
            POST_EXCLUDE_REGEXES
                ".*[Ww]indows.*" ".*System32.*")
    endif()
endif()

set(CPACK_PACKAGE_NAME "RISC-V-Simulator")
set(CPACK_PACKAGE_VENDOR "RISC-V Simulator Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simulador educacional de processadores RV32I")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "RISC-V Simulator")
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
    find_program(RV_MAKENSIS_EXECUTABLE makensis)
    if(RV_MAKENSIS_EXECUTABLE)
        list(APPEND CPACK_GENERATOR "NSIS")
    endif()
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop;ZIP")
else()
    set(CPACK_GENERATOR "TGZ;ZIP")
endif()
include(CPack)
