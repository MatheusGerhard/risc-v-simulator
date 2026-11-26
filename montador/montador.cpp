#include "montador.hpp"

#include "../mac1/componentes/memory.hpp"
#include "../mac1/componentes/registers.hpp"
#include "../src/services/simulationLog.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <limits>
#include <regex>
#include <sstream>
#include <string>
#include <utility>

namespace rv32i {
namespace {
std::string_view trim(std::string_view value) noexcept {
    const auto first = value.find_first_not_of(" \t\r\n\f\v");
    if (first == std::string_view::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n\f\v");
    return value.substr(first, last - first + 1);
}

std::string upper(std::string_view value) {
    std::string result(value);
    for (auto& ch : result)
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    return result;
}

int digitValue(char ch) noexcept {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

double parseIntegerPrefix(std::string_view digits, unsigned int radix, bool negative) noexcept {
    double result = 0;
    std::size_t consumed = 0;
    for (const char ch : digits) {
        const int digit = digitValue(ch);
        if (digit < 0 || static_cast<unsigned int>(digit) >= radix) break;
        result = result * static_cast<double>(radix) + static_cast<double>(digit);
        ++consumed;
    }
    if (consumed == 0) return std::numeric_limits<double>::quiet_NaN();
    return negative ? -result : result;
}

std::string numberToString(double number) {
    if (std::isnan(number)) return "NaN";
    if (number == 0.0) return "0";
    if (std::isfinite(number) && std::trunc(number) == number &&
        number >= static_cast<double>(std::numeric_limits<std::int64_t>::min()) &&
        number <= static_cast<double>(std::numeric_limits<std::int64_t>::max()))
        return std::to_string(static_cast<std::int64_t>(number));
    std::ostringstream stream;
    stream << number;
    return stream.str();
}

std::uint32_t jsToUint32(double number) noexcept {
    if (!std::isfinite(number) || number == 0.0) return 0;
    constexpr double modulus = 4294967296.0;
    double reduced = std::fmod(std::trunc(number), modulus);
    if (reduced < 0.0) reduced += modulus;
    return static_cast<std::uint32_t>(reduced);
}

std::string joinOperands(const std::vector<std::string>& operands) {
    std::string result;
    for (std::size_t i = 0; i < operands.size(); ++i) {
        if (i != 0) result += ", ";
        result += operands[i];
    }
    return result;
}

std::string assembleError(const MacroInstruction& instruction, std::string_view message) {
    return "Erro na instrução '" + instruction.originalLine + "' (PC " +
           std::to_string(instruction.byteAddress) + "): " + std::string(message);
}

std::uint32_t registerOrZero(std::string_view name) {
    const auto parsed = parseRegisterName(name);
    return parsed ? *parsed : 0U;
}
} // namespace

Montador::Montador(std::string assemblyCode) : rawCode(std::move(assemblyCode)) {}

double Montador::parseImmediate(std::string_view token) const noexcept {
    if (token.empty()) return 0.0;
    const auto clean = trim(token);
    if (clean.empty()) return 0.0;

    if (clean.starts_with("0x") || clean.starts_with("0X") || clean.starts_with("-0x") ||
        clean.starts_with("-0X")) {
        const bool negative = clean.front() == '-';
        const auto start = negative ? 3U : 2U;
        return parseIntegerPrefix(clean.substr(start), 16, negative);
    }
    if (clean.starts_with("0b") || clean.starts_with("0B") || clean.starts_with("-0b") ||
        clean.starts_with("-0B")) {
        const bool negative = clean.front() == '-';
        const auto start = negative ? 3U : 2U;
        return parseIntegerPrefix(clean.substr(start), 2, negative);
    }

    bool negative = false;
    std::size_t start = 0;
    if (clean[start] == '+' || clean[start] == '-') {
        negative = clean[start] == '-';
        ++start;
    }
    return parseIntegerPrefix(clean.substr(start), 10, negative);
}

std::vector<std::string> Montador::splitOperands(std::string_view operands) const {
    std::vector<std::string> result;
    std::size_t start = 0;
    while (start <= operands.size()) {
        const auto comma = operands.find(',', start);
        const auto part = trim(operands.substr(start, comma == std::string_view::npos
                                                         ? operands.size() - start
                                                         : comma - start));
        if (!part.empty()) result.emplace_back(part);
        if (comma == std::string_view::npos) break;
        start = comma + 1;
    }
    return result;
}

std::optional<MemoryOperand> Montador::parseMemoryOperand(std::string_view operand) const {
    static const std::regex memoryPattern(R"(^([+-]?\w+)\s*\(([^)]+)\)$)");
    const std::string value(trim(operand));
    std::smatch match;
    if (!std::regex_match(value, match, memoryPattern)) return std::nullopt;
    return MemoryOperand{match[1].str(), match[2].str()};
}

std::vector<ExpandedInstruction> Montador::expandPseudoInstruction(
    std::string_view opcode, const std::vector<std::string>& operands) const {
    const auto op = upper(opcode);
    const auto operand = [&operands](std::size_t index) -> std::string {
        return index < operands.size() ? operands[index] : std::string{};
    };
    if (op == "NOP") return {{"ADDI", {"x0", "x0", "0"}}};
    if (op == "MV") return {{"ADDI", {operand(0), operand(1), "0"}}};
    if (op == "NOT") return {{"XORI", {operand(0), operand(1), "-1"}}};
    if (op == "NEG") return {{"SUB", {operand(0), "x0", operand(1)}}};
    if (op == "SEQZ") return {{"SLTIU", {operand(0), operand(1), "1"}}};
    if (op == "SNEZ") return {{"SLTU", {operand(0), "x0", operand(1)}}};
    if (op == "SLTZ") return {{"SLT", {operand(0), operand(1), "x0"}}};
    if (op == "SGTZ") return {{"SLT", {operand(0), "x0", operand(1)}}};
    if (op == "BEQZ") return {{"BEQ", {operand(0), "x0", operand(1)}}};
    if (op == "BNEZ") return {{"BNE", {operand(0), "x0", operand(1)}}};
    if (op == "BLEZ") return {{"BGE", {"x0", operand(0), operand(1)}}};
    if (op == "BGEZ") return {{"BGE", {operand(0), "x0", operand(1)}}};
    if (op == "BLTZ") return {{"BLT", {operand(0), "x0", operand(1)}}};
    if (op == "BGTZ") return {{"BLT", {"x0", operand(0), operand(1)}}};
    if (op == "BGT") return {{"BLT", {operand(1), operand(0), operand(2)}}};
    if (op == "BLE") return {{"BGE", {operand(1), operand(0), operand(2)}}};
    if (op == "BGTU") return {{"BLTU", {operand(1), operand(0), operand(2)}}};
    if (op == "BLEU") return {{"BGEU", {operand(1), operand(0), operand(2)}}};
    if (op == "J") return {{"JAL", {"x0", operand(0)}}};
    if (op == "JR") return {{"JALR", {"x0", operand(0), "0"}}};
    if (op == "RET") return {{"JALR", {"x0", "ra", "0"}}};
    if (op == "LI") {
        const double value = parseImmediate(operand(1));
        if (value >= -2048.0 && value <= 2047.0)
            return {{"ADDI", {operand(0), "x0", numberToString(value)}}};
        const auto upperValue = (jsToUint32(value + 2048.0) >> 12U);
        const auto shiftedUpper = upperValue << 12U;
        const auto lowerValue = std::bit_cast<std::int32_t>(jsToUint32(value - shiftedUpper));
        return {{"LUI", {operand(0), std::to_string(upperValue)}},
                {"ADDI", {operand(0), operand(0), std::to_string(lowerValue)}}};
    }
    return {{op, operands}};
}

const std::vector<MacroInstruction>& Montador::primeira_passagem() {
    macroinstrucoes.clear();
    microinstrucoes.clear();
    labelTable.clear();
    std::uint32_t currentByteAddress = 0;
    std::istringstream source(rawCode);
    std::string line;

    while (std::getline(source, line)) {
        const auto comment = line.find_first_of(";#");
        if (comment != std::string::npos) line.erase(comment);
        std::string working(trim(line));
        if (working.empty()) continue;

        const auto colon = working.find(':');
        if (colon != std::string::npos) {
            const std::string label(trim(std::string_view(working).substr(0, colon)));
            working = std::string(trim(std::string_view(working).substr(colon + 1)));
            if (!label.empty()) labelTable[upper(label)] = currentByteAddress;
        }
        if (working.empty()) continue;

        const auto mnemonicEnd = working.find_first_of(" \t\r\n\f\v");
        const std::string opcode = upper(std::string_view(working).substr(0, mnemonicEnd));
        const std::string operandsText = mnemonicEnd == std::string::npos
                                             ? std::string{}
                                             : std::string(trim(std::string_view(working).substr(mnemonicEnd)));
        const auto operands = splitOperands(operandsText);
        const auto expanded = expandPseudoInstruction(opcode, operands);
        for (const auto& item : expanded) {
            macroinstrucoes.push_back({working, currentByteAddress, currentByteAddress / 4U,
                                       item.opcode, item.operands, std::nullopt, 0, 0, 0, 0.0});
            currentByteAddress += 4U;
        }
    }
    return macroinstrucoes;
}

std::optional<std::string> Montador::segunda_passagem() {
    microinstrucoes.clear();
    for (auto& instruction : macroinstrucoes) {
        const auto& op = instruction.opcode;
        const auto& ops = instruction.operands;
        const auto pc = instruction.byteAddress;
        const auto operand = [&ops](std::size_t index) -> std::string_view {
            return index < ops.size() ? std::string_view(ops[index]) : std::string_view{};
        };
        try {
            if (op == "ADD" || op == "SUB" || op == "SLL" || op == "SLT" || op == "SLTU" ||
                op == "XOR" || op == "SRL" || op == "SRA" || op == "OR" || op == "AND") {
                if (ops.size() < 3)
                    return assembleError(instruction, op + " requer 3 registradores: rd, rs1, rs2");
                instruction.rd = registerOrZero(operand(0));
                instruction.rs1 = registerOrZero(operand(1));
                instruction.rs2 = registerOrZero(operand(2));
            } else if (op == "ADDI" || op == "SLLI" || op == "SLTI" || op == "SLTIU" ||
                       op == "XORI" || op == "SRLI" || op == "SRAI" || op == "ORI" || op == "ANDI") {
                if (ops.size() < 3)
                    return assembleError(instruction, op + " requer: rd, rs1, imm");
                instruction.rd = registerOrZero(operand(0));
                instruction.rs1 = registerOrZero(operand(1));
                instruction.imm = parseImmediate(operand(2));
            } else if (op == "LB" || op == "LH" || op == "LW" || op == "LBU" || op == "LHU") {
                instruction.rd = registerOrZero(operand(0));
                if (ops.size() == 2) {
                    const auto memory = parseMemoryOperand(operand(1));
                    if (!memory)
                        return assembleError(instruction, "Formato de memória inválido para " + op +
                                                             ": " + ops[1]);
                    instruction.imm = parseImmediate(memory->offset);
                    instruction.rs1 = registerOrZero(memory->baseReg);
                } else if (ops.size() == 3) {
                    instruction.rs1 = registerOrZero(operand(1));
                    instruction.imm = parseImmediate(operand(2));
                }
            } else if (op == "SB" || op == "SH" || op == "SW") {
                instruction.rs2 = registerOrZero(operand(0));
                if (ops.size() == 2) {
                    const auto memory = parseMemoryOperand(operand(1));
                    if (!memory)
                        return assembleError(instruction, "Formato de memória inválido para " + op +
                                                             ": " + ops[1]);
                    instruction.imm = parseImmediate(memory->offset);
                    instruction.rs1 = registerOrZero(memory->baseReg);
                } else if (ops.size() == 3) {
                    instruction.rs1 = registerOrZero(operand(1));
                    instruction.imm = parseImmediate(operand(2));
                }
            } else if (op == "BEQ" || op == "BNE" || op == "BLT" || op == "BGE" ||
                       op == "BLTU" || op == "BGEU") {
                if (ops.size() < 3)
                    return assembleError(instruction, op + " requer: rs1, rs2, label");
                instruction.rs1 = registerOrZero(operand(0));
                instruction.rs2 = registerOrZero(operand(1));
                const std::string target = upper(operand(2));
                if (const auto label = labelTable.find(target); label != labelTable.end()) {
                    instruction.imm = static_cast<double>(label->second) - static_cast<double>(pc);
                    instruction.labelRef = target;
                } else {
                    instruction.imm = parseImmediate(operand(2));
                }
            } else if (op == "LUI" || op == "AUIPC") {
                if (ops.size() < 2)
                    return assembleError(instruction, op + " requer: rd, imm");
                instruction.rd = registerOrZero(operand(0));
                instruction.imm = parseImmediate(operand(1));
            } else if (op == "JAL") {
                std::string target;
                if (ops.size() == 1) {
                    instruction.rd = 1;
                    target = upper(ops[0]);
                } else {
                    instruction.rd = registerOrZero(operand(0));
                    if (ops.size() < 2)
                        return assembleError(instruction, "Cannot read properties of undefined (reading 'toUpperCase')");
                    target = upper(ops[1]);
                }
                if (const auto label = labelTable.find(target); label != labelTable.end()) {
                    instruction.imm = static_cast<double>(label->second) - static_cast<double>(pc);
                    instruction.labelRef = target;
                } else {
                    instruction.imm = parseImmediate(target);
                }
            } else if (op == "JALR") {
                if (ops.size() == 1) {
                    instruction.rd = 1;
                    instruction.rs1 = registerOrZero(operand(0));
                    instruction.imm = 0;
                } else if (ops.size() == 2) {
                    instruction.rd = registerOrZero(operand(0));
                    if (const auto memory = parseMemoryOperand(operand(1))) {
                        instruction.imm = parseImmediate(memory->offset);
                        instruction.rs1 = registerOrZero(memory->baseReg);
                    } else {
                        instruction.rs1 = registerOrZero(operand(1));
                        instruction.imm = 0;
                    }
                } else if (ops.size() == 3) {
                    instruction.rd = registerOrZero(operand(0));
                    instruction.rs1 = registerOrZero(operand(1));
                    instruction.imm = parseImmediate(operand(2));
                }
            } else if (op == "ECALL" || op == "EBREAK") {
                instruction.rd = 0;
                instruction.rs1 = 0;
                instruction.imm = op == "EBREAK" ? 1.0 : 0.0;
            } else {
                return assembleError(instruction, "Instrução não reconhecida: " + op);
            }

            const auto word = detecta_tipo_instrucao(op, instruction.rd, instruction.rs1,
                                                     instruction.rs2, instruction.imm);
            if (!word) return assembleError(instruction, "Falha ao codificar instrução: " + op);
            microinstrucoes.push_back(*word);
        } catch (const std::exception& error) {
            return assembleError(instruction, error.what());
        }
    }
    return std::nullopt;
}

std::optional<std::string> Montador::main() {
    (void)primeira_passagem();
    return segunda_passagem();
}

void Montador::preencherMemoria(Memory& memory, SimulationLog& log) const {
    std::vector<std::pair<std::uint32_t, std::string>> catalog;
    catalog.reserve(macroinstrucoes.size());
    const auto count = std::min(macroinstrucoes.size(), microinstrucoes.size());
    for (std::size_t i = 0; i < count; ++i) {
        const auto& instruction = macroinstrucoes[i];
        std::string text = instruction.opcode;
        if (!instruction.operands.empty()) text += " " + joinOperands(instruction.operands);
        catalog.emplace_back(instruction.byteAddress, std::move(text));
        memory.writeMontador(instruction.byteAddress, microinstrucoes[i]);
    }
    log.registerMacroInstructions(std::move(catalog));
}

} // namespace rv32i
