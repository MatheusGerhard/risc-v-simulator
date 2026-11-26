#include "macroinstrucoes.hpp"

#include <algorithm>
#include <bit>
#include <cctype>
#include <cmath>
#include <string>

namespace rv32i {
namespace {
std::string upper(std::string_view opcode) {
    std::string result(opcode);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return result;
}

std::uint32_t toUint32(double number) noexcept {
    if (!std::isfinite(number) || number == 0.0) return 0;
    constexpr double modulus = 4294967296.0;
    double reduced = std::fmod(std::trunc(number), modulus);
    if (reduced < 0.0) reduced += modulus;
    return static_cast<std::uint32_t>(reduced);
}
} // namespace

std::optional<std::uint32_t> detecta_tipo_instrucao(std::string_view opcode,
                                                    std::uint32_t rd, std::uint32_t rs1,
                                                    std::uint32_t rs2, double imm) noexcept {
    try {
        const auto op = upper(opcode);
        const auto intImm = std::bit_cast<std::int32_t>(toUint32(imm));
        const auto encodedImm = static_cast<std::int64_t>(intImm);
        if (op == "ADD") return encodeRType(0x00, rs2, rs1, 0x0, rd, OPCODES::R_TYPE);
        if (op == "SUB") return encodeRType(0x20, rs2, rs1, 0x0, rd, OPCODES::R_TYPE);
        if (op == "SLL") return encodeRType(0x00, rs2, rs1, 0x1, rd, OPCODES::R_TYPE);
        if (op == "SLT") return encodeRType(0x00, rs2, rs1, 0x2, rd, OPCODES::R_TYPE);
        if (op == "SLTU") return encodeRType(0x00, rs2, rs1, 0x3, rd, OPCODES::R_TYPE);
        if (op == "XOR") return encodeRType(0x00, rs2, rs1, 0x4, rd, OPCODES::R_TYPE);
        if (op == "SRL") return encodeRType(0x00, rs2, rs1, 0x5, rd, OPCODES::R_TYPE);
        if (op == "SRA") return encodeRType(0x20, rs2, rs1, 0x5, rd, OPCODES::R_TYPE);
        if (op == "OR") return encodeRType(0x00, rs2, rs1, 0x6, rd, OPCODES::R_TYPE);
        if (op == "AND") return encodeRType(0x00, rs2, rs1, 0x7, rd, OPCODES::R_TYPE);

        if (op == "ADDI") return encodeIType(encodedImm, rs1, 0x0, rd, OPCODES::I_ALU);
        if (op == "SLLI") return encodeIType(intImm & 0x1f, rs1, 0x1, rd, OPCODES::I_ALU);
        if (op == "SLTI") return encodeIType(encodedImm, rs1, 0x2, rd, OPCODES::I_ALU);
        if (op == "SLTIU") return encodeIType(encodedImm, rs1, 0x3, rd, OPCODES::I_ALU);
        if (op == "XORI") return encodeIType(encodedImm, rs1, 0x4, rd, OPCODES::I_ALU);
        if (op == "SRLI") return encodeIType(intImm & 0x1f, rs1, 0x5, rd, OPCODES::I_ALU);
        if (op == "SRAI") return encodeIType(0x400 | (intImm & 0x1f), rs1, 0x5, rd, OPCODES::I_ALU);
        if (op == "ORI") return encodeIType(encodedImm, rs1, 0x6, rd, OPCODES::I_ALU);
        if (op == "ANDI") return encodeIType(encodedImm, rs1, 0x7, rd, OPCODES::I_ALU);

        if (op == "LB") return encodeIType(encodedImm, rs1, 0x0, rd, OPCODES::I_LOAD);
        if (op == "LH") return encodeIType(encodedImm, rs1, 0x1, rd, OPCODES::I_LOAD);
        if (op == "LW") return encodeIType(encodedImm, rs1, 0x2, rd, OPCODES::I_LOAD);
        if (op == "LBU") return encodeIType(encodedImm, rs1, 0x4, rd, OPCODES::I_LOAD);
        if (op == "LHU") return encodeIType(encodedImm, rs1, 0x5, rd, OPCODES::I_LOAD);

        if (op == "JALR") return encodeIType(encodedImm, rs1, 0x0, rd, OPCODES::I_JALR);
        if (op == "ECALL") return encodeIType(0, 0, 0x0, 0, OPCODES::I_SYS);
        if (op == "EBREAK") return encodeIType(1, 0, 0x0, 0, OPCODES::I_SYS);

        if (op == "SB") return encodeSType(encodedImm, rs2, rs1, 0x0, OPCODES::S_TYPE);
        if (op == "SH") return encodeSType(encodedImm, rs2, rs1, 0x1, OPCODES::S_TYPE);
        if (op == "SW") return encodeSType(encodedImm, rs2, rs1, 0x2, OPCODES::S_TYPE);

        if (op == "BEQ") return encodeBType(encodedImm, rs2, rs1, 0x0, OPCODES::B_TYPE);
        if (op == "BNE") return encodeBType(encodedImm, rs2, rs1, 0x1, OPCODES::B_TYPE);
        if (op == "BLT") return encodeBType(encodedImm, rs2, rs1, 0x4, OPCODES::B_TYPE);
        if (op == "BGE") return encodeBType(encodedImm, rs2, rs1, 0x5, OPCODES::B_TYPE);
        if (op == "BLTU") return encodeBType(encodedImm, rs2, rs1, 0x6, OPCODES::B_TYPE);
        if (op == "BGEU") return encodeBType(encodedImm, rs2, rs1, 0x7, OPCODES::B_TYPE);

        if (op == "LUI" || op == "AUIPC") {
            const auto uImm = imm > 0xfffff ? static_cast<std::int64_t>(toUint32(imm)) : encodedImm;
            return encodeUType(uImm, rd, op == "LUI" ? OPCODES::U_LUI : OPCODES::U_AUIPC);
        }
        if (op == "JAL") return encodeJType(encodedImm, rd, OPCODES::J_JAL);
    } catch (...) {
        return std::nullopt;
    }
    return std::nullopt;
}

} // namespace rv32i
