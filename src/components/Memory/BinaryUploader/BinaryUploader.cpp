#include "BinaryUploader.hpp"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <nfd.h>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cfloat>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

namespace rv32i {
namespace {
std::string_view trim(std::string_view text) noexcept {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
    return text;
}

void setError(MacContext& context, std::string message) {
    context.binaryStatus = std::move(message);
}
}

std::optional<std::uint32_t> parseBaseAddress(std::string_view input) noexcept {
    auto text = trim(input);
    if (text.empty()) return std::nullopt;
    int base = 10;
    if (text.size() >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        text.remove_prefix(2);
        base = 16;
    }
    if (text.empty()) return std::nullopt;
    std::uint64_t value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, base);
    if (error != std::errc{} || end != text.data() + text.size() ||
        value > std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
    return static_cast<std::uint32_t>(value);
}

bool loadBinaryFile(MacContext& context, const std::filesystem::path& path) {
    auto& machine = context.selected();
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (extension != ".bin") {
        setError(context, "Selecione um arquivo .bin bruto (flat binary).");
        return false;
    }
    const auto base = parseBaseAddress(context.binaryBaseAddress);
    if (!base) {
        setError(context, "Endereço base inválido. Use decimal ou hexadecimal 0x… dentro de 32 bits.");
        return false;
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        setError(context, "Não foi possível ler o arquivo .bin selecionado.");
        return false;
    }
    const std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(input),
                                          std::istreambuf_iterator<char>()};

    machine.memory.clearMemory();
    const auto loaded = machine.memory.loadRawBinary(bytes, *base);
    machine.clock.resetClock();
    machine.uc.hardReset(*base);
    context.ipsMeter.reset();
    context.binaryStatus = path.filename().string() + " · " + std::to_string(loaded) +
                           " bytes carregados em 0x";
    constexpr char digits[] = "0123456789ABCDEF";
    auto value = *base;
    std::string address;
    do {
        address.push_back(digits[value & 0xFU]);
        value >>= 4U;
    } while (value != 0);
    std::reverse(address.begin(), address.end());
    context.binaryStatus += address + ". PC reiniciado.";
    return true;
}

void drawBinaryUploader(MacContext& context) {
    ImGui::SeparatorText("Binário .bin");
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText("Endereço base", &context.binaryBaseAddress);
    if (ImGui::Button("Upload .bin", ImVec2(-FLT_MIN, 0))) {
        if (NFD_Init() != NFD_OKAY) {
            const char* error = NFD_GetError();
            setError(context, error ? error : "Falha ao inicializar o seletor de arquivos.");
        } else {
            nfdu8char_t* selectedPath = nullptr;
            const nfdu8filteritem_t filters[]{{"Binary files", "bin"}};
            const auto result = NFD_OpenDialogU8(&selectedPath, filters, 1, nullptr);
            if (result == NFD_OKAY) {
                const auto* utf8 = reinterpret_cast<const char8_t*>(selectedPath);
                (void)loadBinaryFile(context, std::filesystem::path(std::u8string(utf8)));
                NFD_FreePathU8(selectedPath);
            } else if (result == NFD_ERROR) {
                const char* error = NFD_GetError();
                setError(context, error ? error : "Falha ao abrir o seletor de arquivos.");
            }
            NFD_Quit();
        }
    }
    ImGui::TextDisabled("Arraste um arquivo .bin para a janela para carregar.");
    if (!context.binaryStatus.empty()) ImGui::TextWrapped("%s", context.binaryStatus.c_str());
}

} // namespace rv32i
