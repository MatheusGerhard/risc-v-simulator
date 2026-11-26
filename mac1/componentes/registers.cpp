#include "registers.hpp"

#include <algorithm>
#include <bit>
#include <charconv>
#include <cctype>
#include <cstdio>

namespace rv32i {
namespace {
std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string lower(std::string_view value) {
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}
} // namespace

std::optional<std::uint8_t> parseRegisterName(std::string_view name) {
    name = trim(name);
    if (!name.empty() && name.back() == ',') name.remove_suffix(1);
    const auto cleaned = lower(trim(name));
    if (cleaned.size() > 1 && cleaned.front() == 'x') {
        unsigned int index = 0;
        const auto [ptr, error] = std::from_chars(cleaned.data() + 1,
                                                  cleaned.data() + cleaned.size(), index);
        if (error == std::errc{} && ptr == cleaned.data() + cleaned.size() && index < 32)
            return static_cast<std::uint8_t>(index);
    }
    if (cleaned == "fp") return 8;
    for (std::size_t i = 0; i < ABI_NAMES.size(); ++i) {
        if (cleaned == ABI_NAMES[i]) return static_cast<std::uint8_t>(i);
    }
    return std::nullopt;
}

std::string toHex32(std::uint32_t value) {
    char buffer[11]{};
    std::snprintf(buffer, sizeof(buffer), "0x%08X", value);
    return buffer;
}

std::string toBinary32(std::uint32_t value) {
    std::string result(32, '0');
    for (unsigned int bit = 0; bit < 32; ++bit)
        result[31U - bit] = ((value >> bit) & 1U) ? '1' : '0';
    return result;
}

void Registers::write(std::uint8_t reg, std::int32_t value) noexcept {
    if (reg > 0 && reg < data_.size()) data_[reg] = static_cast<std::uint32_t>(value);
}

void Registers::write(std::string_view reg, std::int32_t value) {
    if (const auto index = parseRegisterName(reg)) write(*index, value);
}

std::int32_t Registers::readInt(std::uint8_t reg) const noexcept {
    return reg < data_.size() ? std::bit_cast<std::int32_t>(data_[reg]) : 0;
}

std::int32_t Registers::readInt(std::string_view reg) const noexcept {
    if (const auto index = parseRegisterName(reg)) return readInt(*index);
    return 0;
}

std::uint32_t Registers::readUInt(std::uint8_t reg) const noexcept {
    return reg < data_.size() ? data_[reg] : 0;
}

std::uint32_t Registers::readUInt(std::string_view reg) const noexcept {
    if (const auto index = parseRegisterName(reg)) return readUInt(*index);
    return 0;
}

std::string Registers::read(std::uint8_t reg) const { return toBinary32(readUInt(reg)); }
std::string Registers::read(std::string_view reg) const { return toBinary32(readUInt(reg)); }
std::string Registers::readHex(std::uint8_t reg) const { return toHex32(readUInt(reg)); }
std::uint32_t Registers::readPC() const noexcept { return pc_; }
std::string Registers::readPCBinary() const { return toBinary32(pc_); }
std::string Registers::readPCHex() const { return toHex32(pc_); }
void Registers::writePC(std::uint32_t value) noexcept { pc_ = value; }

std::array<std::uint32_t, 32> Registers::readAll() const noexcept {
    auto copy = data_;
    copy[0] = 0;
    return copy;
}

std::vector<RegisterInfo> Registers::getAllWithMeta() const {
    std::vector<RegisterInfo> result;
    result.reserve(data_.size());
    for (std::uint8_t index = 0; index < data_.size(); ++index) {
        const auto value = readUInt(index);
        const auto id = "x" + std::to_string(index);
        const auto abi = std::string(ABI_NAMES[index]);
        result.push_back({index, id, id, abi, id + " (" + abi + ")", toBinary32(value),
                          toHex32(value), readInt(index)});
    }
    return result;
}

void Registers::clear() noexcept {
    data_.fill(0);
    pc_ = 0;
}

} // namespace rv32i
