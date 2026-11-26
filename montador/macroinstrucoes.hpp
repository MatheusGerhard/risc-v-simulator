#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace rv32i {

namespace OPCODES {
inline constexpr std::uint32_t R_TYPE = 0b0110011;
inline constexpr std::uint32_t I_ALU = 0b0010011;
inline constexpr std::uint32_t I_LOAD = 0b0000011;
inline constexpr std::uint32_t I_JALR = 0b1100111;
inline constexpr std::uint32_t I_SYS = 0b1110011;
inline constexpr std::uint32_t S_TYPE = 0b0100011;
inline constexpr std::uint32_t B_TYPE = 0b1100011;
inline constexpr std::uint32_t U_LUI = 0b0110111;
inline constexpr std::uint32_t U_AUIPC = 0b0010111;
inline constexpr std::uint32_t J_JAL = 0b1101111;
} // namespace OPCODES

[[nodiscard]] constexpr std::uint32_t encodeRType(std::uint32_t funct7, std::uint32_t rs2,
                                                   std::uint32_t rs1, std::uint32_t funct3,
                                                   std::uint32_t rd, std::uint32_t opcode) noexcept {
    return ((funct7 & 0x7fU) << 25U) | ((rs2 & 0x1fU) << 20U) | ((rs1 & 0x1fU) << 15U) |
           ((funct3 & 7U) << 12U) | ((rd & 0x1fU) << 7U) | (opcode & 0x7fU);
}

[[nodiscard]] constexpr std::uint32_t encodeIType(std::int64_t imm, std::uint32_t rs1,
                                                   std::uint32_t funct3, std::uint32_t rd,
                                                   std::uint32_t opcode) noexcept {
    return ((static_cast<std::uint32_t>(imm) & 0xfffU) << 20U) | ((rs1 & 0x1fU) << 15U) |
           ((funct3 & 7U) << 12U) | ((rd & 0x1fU) << 7U) | (opcode & 0x7fU);
}

[[nodiscard]] constexpr std::uint32_t encodeSType(std::int64_t imm, std::uint32_t rs2,
                                                   std::uint32_t rs1, std::uint32_t funct3,
                                                   std::uint32_t opcode) noexcept {
    const auto bits = static_cast<std::uint32_t>(imm);
    const auto imm11_5 = (bits >> 5U) & 0x7fU;
    const auto imm4_0 = bits & 0x1fU;
    return (imm11_5 << 25U) | ((rs2 & 0x1fU) << 20U) | ((rs1 & 0x1fU) << 15U) |
           ((funct3 & 7U) << 12U) | (imm4_0 << 7U) | (opcode & 0x7fU);
}

[[nodiscard]] constexpr std::uint32_t encodeBType(std::int64_t imm, std::uint32_t rs2,
                                                   std::uint32_t rs1, std::uint32_t funct3,
                                                   std::uint32_t opcode) noexcept {
    const auto bits = static_cast<std::uint32_t>(imm);
    const auto imm12 = (bits >> 12U) & 1U;
    const auto imm11 = (bits >> 11U) & 1U;
    const auto imm10_5 = (bits >> 5U) & 0x3fU;
    const auto imm4_1 = (bits >> 1U) & 0xfU;
    return (imm12 << 31U) | (imm10_5 << 25U) | ((rs2 & 0x1fU) << 20U) |
           ((rs1 & 0x1fU) << 15U) | ((funct3 & 7U) << 12U) | (imm4_1 << 8U) |
           (imm11 << 7U) | (opcode & 0x7fU);
}

[[nodiscard]] constexpr std::uint32_t encodeUType(std::int64_t imm, std::uint32_t rd,
                                                   std::uint32_t opcode) noexcept {
    const auto value = static_cast<std::uint32_t>(imm);
    const auto imm20 = imm > 0xfffff ? (value >> 12U) & 0xfffffU : value & 0xfffffU;
    return (imm20 << 12U) | ((rd & 0x1fU) << 7U) | (opcode & 0x7fU);
}

[[nodiscard]] constexpr std::uint32_t encodeJType(std::int64_t imm, std::uint32_t rd,
                                                   std::uint32_t opcode) noexcept {
    const auto bits = static_cast<std::uint32_t>(imm);
    const auto imm20 = (bits >> 20U) & 1U;
    const auto imm10_1 = (bits >> 1U) & 0x3ffU;
    const auto imm11 = (bits >> 11U) & 1U;
    const auto imm19_12 = (bits >> 12U) & 0xffU;
    return (imm20 << 31U) | (imm10_1 << 21U) | (imm11 << 20U) | (imm19_12 << 12U) |
           ((rd & 0x1fU) << 7U) | (opcode & 0x7fU);
}

[[nodiscard]] std::optional<std::uint32_t> detecta_tipo_instrucao(
    std::string_view opcode, std::uint32_t rd, std::uint32_t rs1, std::uint32_t rs2,
    double imm) noexcept;

} // namespace rv32i
