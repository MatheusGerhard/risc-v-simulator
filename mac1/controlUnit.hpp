#pragma once

#include "componentes/alu.hpp"
#include "componentes/memory.hpp"
#include "componentes/registers.hpp"
#include "../src/services/simulationLog.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace rv32i {

struct DecodedInstruction {
    std::uint8_t opcode{};
    std::uint8_t rd{};
    std::uint8_t funct3{};
    std::uint8_t rs1{};
    std::uint8_t rs2{};
    std::uint8_t funct7{};
    std::int32_t immI{};
    std::int32_t immS{};
    std::int32_t immB{};
    std::int32_t immU{};
    std::int32_t immJ{};
    std::int32_t imm{};
    std::string format{"UNKNOWN"};
    std::string mnemonic{"NOP"};
};

struct Estado {
    std::uint8_t subciclo{1};
    std::uint64_t instrucoesConcluidas{};
    std::uint64_t ciclos{};
    std::string stage{"RESET"};
    std::string mir{"RESET"};
    std::string mpc{"NOP"};
    std::string micro{"-"};
    std::uint32_t pc{};
    std::uint32_t ir{};
    std::uint32_t mar{};
    std::uint32_t mbr{};
    std::int32_t latA{};
    std::int32_t latB{};
    std::int32_t aluRes{};
    bool aluZ{true};
    bool aluN{};
    bool branchTaken{};
    bool jumpTaken{};
    DecodedInstruction decoded;
    std::array<std::uint32_t, 32> registers{};
    std::vector<RegisterInfo> registersList;
};

class ControlUnit {
public:
    explicit ControlUnit(Memory& memory,
                         SimulationLog& log = SimulationLog::instance());

    void setCallback(std::function<void(const Estado&)> callback);
    [[nodiscard]] DecodedInstruction decodeInstruction(std::uint32_t instruction) const;
    virtual bool rodarCiclo(std::uint8_t subciclo, std::uint64_t ciclos);
    [[nodiscard]] virtual std::uint64_t cacheHits() const noexcept;
    [[nodiscard]] virtual std::uint64_t cacheMisses() const noexcept;
    [[nodiscard]] virtual std::uint64_t cachePenaltyCycles() const noexcept;
    [[nodiscard]] std::uint64_t getCompletedInstructions() const noexcept;
    [[nodiscard]] Estado getEstado(std::uint8_t subciclo = 1, std::uint64_t ciclos = 0) const;
    void hardReset(std::uint32_t baseAddress = 0);
    virtual void reset();

    [[nodiscard]] Registers& registradores() noexcept;
    [[nodiscard]] const Registers& registradores() const noexcept;
    [[nodiscard]] Memory& ram() noexcept;
    [[nodiscard]] const Memory& ram() const noexcept;

protected:
    [[nodiscard]] virtual std::int32_t readMemoryByte(std::uint32_t address, bool signedValue);
    [[nodiscard]] virtual std::int32_t readMemoryHalf(std::uint32_t address, bool signedValue);
    [[nodiscard]] virtual std::int32_t readMemoryWord(std::uint32_t address);
    [[nodiscard]] virtual std::uint32_t readMemoryWordUnsigned(std::uint32_t address);
    virtual void writeMemoryByte(std::uint32_t address, std::uint8_t value);
    virtual void writeMemoryHalf(std::uint32_t address, std::uint16_t value);
    virtual void writeMemoryWord(std::uint32_t address, std::uint32_t value);

    Registers regs_;
    Memory& ram_;
    ArithmeticLogicUnit alu_;
    SimulationLog& log_;
    std::function<void(const Estado&)> onEstadoChange_;

private:
    void notify(std::uint8_t subciclo, std::uint64_t ciclos) const;

    std::uint32_t ir_{};
    std::uint32_t currentPC_{};
    std::uint32_t pcNext_{};
    std::uint32_t mar_{};
    std::uint32_t mbr_{};
    std::int32_t mdr_{};
    std::int32_t latA_{};
    std::int32_t latB_{};
    std::int32_t aluOut_{};
    DecodedInstruction decoded_;
    bool branchTaken_{};
    std::uint32_t branchTarget_{};
    bool jumpTaken_{};
    std::uint32_t jumpTarget_{};
    std::uint64_t instrucoesConcluidas_{};
    std::string stageName_{"RESET"};
    std::string micro_{"-"};
};

} // namespace rv32i
