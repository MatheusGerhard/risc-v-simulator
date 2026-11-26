#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rv32i {

inline constexpr std::array<std::string_view, 32> ABI_NAMES{
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1", "a0", "a1",
    "a2", "a3", "a4", "a5", "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

[[nodiscard]] std::optional<std::uint8_t> parseRegisterName(std::string_view name);
[[nodiscard]] std::string toHex32(std::uint32_t value);
[[nodiscard]] std::string toBinary32(std::uint32_t value);

struct RegisterInfo {
    std::uint8_t index{};
    std::string id;
    std::string name;
    std::string abi;
    std::string label;
    std::string bin;
    std::string hex;
    std::int32_t dec{};
};

class Registers {
public:
    void write(std::uint8_t reg, std::int32_t value) noexcept;
    void write(std::string_view reg, std::int32_t value);
    [[nodiscard]] std::int32_t readInt(std::uint8_t reg) const noexcept;
    [[nodiscard]] std::int32_t readInt(std::string_view reg) const noexcept;
    [[nodiscard]] std::uint32_t readUInt(std::uint8_t reg) const noexcept;
    [[nodiscard]] std::uint32_t readUInt(std::string_view reg) const noexcept;
    [[nodiscard]] std::string read(std::uint8_t reg) const;
    [[nodiscard]] std::string read(std::string_view reg) const;
    [[nodiscard]] std::string readHex(std::uint8_t reg) const;
    [[nodiscard]] std::uint32_t readPC() const noexcept;
    [[nodiscard]] std::string readPCBinary() const;
    [[nodiscard]] std::string readPCHex() const;
    void writePC(std::uint32_t value) noexcept;
    [[nodiscard]] std::array<std::uint32_t, 32> readAll() const noexcept;
    [[nodiscard]] std::vector<RegisterInfo> getAllWithMeta() const;
    void clear() noexcept;

private:
    std::array<std::uint32_t, 32> data_{};
    std::uint32_t pc_{};
};

} // namespace rv32i
