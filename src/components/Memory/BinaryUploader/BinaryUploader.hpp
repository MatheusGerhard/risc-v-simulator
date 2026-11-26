#pragma once

#include "../../../context/MacContext.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string_view>

namespace rv32i {

[[nodiscard]] std::optional<std::uint32_t> parseBaseAddress(std::string_view text) noexcept;
[[nodiscard]] bool loadBinaryFile(MacContext& context, const std::filesystem::path& path);
void drawBinaryUploader(MacContext& context);

} // namespace rv32i
