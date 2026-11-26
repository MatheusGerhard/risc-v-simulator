#pragma once

#include "macroinstrucoes.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace rv32i {

class Memory;
class SimulationLog;

struct ExpandedInstruction {
    std::string opcode;
    std::vector<std::string> operands;
};

struct MacroInstruction {
    std::string originalLine;
    std::uint32_t byteAddress{};
    std::uint32_t rotulo_index{};
    std::string opcode;
    std::vector<std::string> operands;
    std::optional<std::string> labelRef;
    std::uint32_t rd{};
    std::uint32_t rs1{};
    std::uint32_t rs2{};
    double imm{};
};

struct MemoryOperand {
    std::string offset;
    std::string baseReg;
};

class Montador {
public:
    explicit Montador(std::string assemblyCode = {});

    [[nodiscard]] double parseImmediate(std::string_view token) const noexcept;
    [[nodiscard]] std::vector<std::string> splitOperands(std::string_view operands) const;
    [[nodiscard]] std::optional<MemoryOperand> parseMemoryOperand(std::string_view operand) const;
    [[nodiscard]] std::vector<ExpandedInstruction> expandPseudoInstruction(
        std::string_view opcode, const std::vector<std::string>& operands) const;

    [[nodiscard]] const std::vector<MacroInstruction>& primeira_passagem();
    [[nodiscard]] std::optional<std::string> segunda_passagem();
    [[nodiscard]] std::optional<std::string> main();
    void preencherMemoria(Memory& memory, SimulationLog& log) const;

    std::string rawCode;
    std::vector<MacroInstruction> macroinstrucoes;
    std::vector<std::uint32_t> microinstrucoes;
    std::unordered_map<std::string, std::uint32_t> labelTable;
};

} // namespace rv32i
