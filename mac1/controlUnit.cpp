#include "controlUnit.hpp"

#include <bit>
#include <utility>

namespace rv32i {
namespace {
std::int32_t signExtend(std::uint32_t value, unsigned int bits) noexcept {
    const std::uint32_t sign = 1U << (bits - 1U);
    const std::uint32_t mask = (1U << bits) - 1U;
    value &= mask;
    if (value & sign) value |= ~mask;
    return std::bit_cast<std::int32_t>(value);
}

std::int32_t wrapAdd(std::int32_t a, std::int32_t b) noexcept {
    return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(a) +
                                       std::bit_cast<std::uint32_t>(b));
}

} // namespace

ControlUnit::ControlUnit(Memory& memory, SimulationLog& log) : ram_(memory), log_(log) {}

void ControlUnit::setCallback(std::function<void(const Estado&)> callback) {
    onEstadoChange_ = std::move(callback);
}

DecodedInstruction ControlUnit::decodeInstruction(std::uint32_t ir) const {
    DecodedInstruction result;
    result.opcode = static_cast<std::uint8_t>(ir & 0x7fU);
    result.rd = static_cast<std::uint8_t>((ir >> 7U) & 0x1fU);
    result.funct3 = static_cast<std::uint8_t>((ir >> 12U) & 7U);
    result.rs1 = static_cast<std::uint8_t>((ir >> 15U) & 0x1fU);
    result.rs2 = static_cast<std::uint8_t>((ir >> 20U) & 0x1fU);
    result.funct7 = static_cast<std::uint8_t>((ir >> 25U) & 0x7fU);
    result.immI = signExtend(ir >> 20U, 12);
    const auto rawS = ((ir >> 25U) << 5U) | ((ir >> 7U) & 0x1fU);
    result.immS = signExtend(rawS, 12);
    const auto rawB = (((ir >> 31U) & 1U) << 12U) | (((ir >> 7U) & 1U) << 11U) |
                      (((ir >> 25U) & 0x3fU) << 5U) | (((ir >> 8U) & 0xfU) << 1U);
    result.immB = signExtend(rawB, 13);
    result.immU = std::bit_cast<std::int32_t>(ir & 0xfffff000U);
    const auto rawJ = (((ir >> 31U) & 1U) << 20U) | (((ir >> 12U) & 0xffU) << 12U) |
                      (((ir >> 20U) & 1U) << 11U) | (((ir >> 21U) & 0x3ffU) << 1U);
    result.immJ = signExtend(rawJ, 21);

    switch (result.opcode) {
    case 0x33:
        result.format = "R-Type";
        switch (result.funct3) {
        case 0: result.mnemonic = result.funct7 == 0x20 ? "SUB" : "ADD"; break;
        case 1: result.mnemonic = "SLL"; break;
        case 2: result.mnemonic = "SLT"; break;
        case 3: result.mnemonic = "SLTU"; break;
        case 4: result.mnemonic = "XOR"; break;
        case 5: result.mnemonic = result.funct7 == 0x20 ? "SRA" : "SRL"; break;
        case 6: result.mnemonic = "OR"; break;
        case 7: result.mnemonic = "AND"; break;
        default: break;
        }
        break;
    case 0x13:
        result.format = "I-Type";
        result.imm = result.immI;
        switch (result.funct3) {
        case 0: result.mnemonic = "ADDI"; break;
        case 1: result.mnemonic = "SLLI"; break;
        case 2: result.mnemonic = "SLTI"; break;
        case 3: result.mnemonic = "SLTIU"; break;
        case 4: result.mnemonic = "XORI"; break;
        case 5: result.mnemonic = result.funct7 == 0x20 ? "SRAI" : "SRLI"; break;
        case 6: result.mnemonic = "ORI"; break;
        case 7: result.mnemonic = "ANDI"; break;
        default: break;
        }
        break;
    case 0x03:
        result.format = "I-Type";
        result.imm = result.immI;
        switch (result.funct3) {
        case 0: result.mnemonic = "LB"; break;
        case 1: result.mnemonic = "LH"; break;
        case 2: result.mnemonic = "LW"; break;
        case 4: result.mnemonic = "LBU"; break;
        case 5: result.mnemonic = "LHU"; break;
        default: break;
        }
        break;
    case 0x67: result.format = "I-Type"; result.imm = result.immI; result.mnemonic = "JALR"; break;
    case 0x23:
        result.format = "S-Type";
        result.imm = result.immS;
        if (result.funct3 == 0) result.mnemonic = "SB";
        else if (result.funct3 == 1) result.mnemonic = "SH";
        else if (result.funct3 == 2) result.mnemonic = "SW";
        break;
    case 0x63:
        result.format = "B-Type";
        result.imm = result.immB;
        switch (result.funct3) {
        case 0: result.mnemonic = "BEQ"; break;
        case 1: result.mnemonic = "BNE"; break;
        case 4: result.mnemonic = "BLT"; break;
        case 5: result.mnemonic = "BGE"; break;
        case 6: result.mnemonic = "BLTU"; break;
        case 7: result.mnemonic = "BGEU"; break;
        default: break;
        }
        break;
    case 0x37: result.format = "U-Type"; result.imm = result.immU; result.mnemonic = "LUI"; break;
    case 0x17: result.format = "U-Type"; result.imm = result.immU; result.mnemonic = "AUIPC"; break;
    case 0x6f: result.format = "J-Type"; result.imm = result.immJ; result.mnemonic = "JAL"; break;
    case 0x73:
        result.format = "I-Type";
        result.imm = result.immI;
        result.mnemonic = result.immI == 1 ? "EBREAK" : "ECALL";
        break;
    default:
        if (ir == 0) result.mnemonic = "NOP / ZERO";
        break;
    }
    return result;
}

bool ControlUnit::rodarCiclo(std::uint8_t sc, std::uint64_t ciclos) {
    switch (sc) {
    case 1:
        currentPC_ = regs_.readPC();
        mar_ = currentPC_;
        ir_ = ram_.readWordUnsigned(currentPC_);
        mbr_ = ir_;
        pcNext_ = currentPC_ + 4U;
        stageName_ = "FETCH";
        micro_ = "IF: PC=" + toHex32(currentPC_) + " IR=" + toHex32(ir_);
        log_.logInstruction(currentPC_, decodeInstruction(ir_).mnemonic, ciclos);
        break;
    case 2:
        decoded_ = decodeInstruction(ir_);
        latA_ = regs_.readInt(decoded_.rs1);
        latB_ = regs_.readInt(decoded_.rs2);
        stageName_ = "DECODE";
        micro_ = "ID: " + decoded_.mnemonic + " (rd=x" + std::to_string(decoded_.rd) +
                 ", rs1=x" + std::to_string(decoded_.rs1) + ", rs2=x" +
                 std::to_string(decoded_.rs2) + ", imm=" + std::to_string(decoded_.imm) + ")";
        break;
    case 3: {
        const auto& d = decoded_;
        const auto a = latA_;
        const auto b = latB_;
        branchTaken_ = false;
        jumpTaken_ = false;
        switch (d.opcode) {
        case 0x33:
            alu_.write(a, b, d.mnemonic);
            aluOut_ = alu_.calcular();
            break;
        case 0x13:
            alu_.write(a, d.imm, d.funct3 == 5 ? (d.funct7 == 0x20 ? "SRA" : "SRL") :
                               d.funct3 == 1 ? "SLL" : d.funct3 == 2 ? "SLT" :
                               d.funct3 == 3 ? "SLTU" : d.funct3 == 4 ? "XOR" :
                               d.funct3 == 6 ? "OR" : d.funct3 == 7 ? "AND" : "ADD");
            aluOut_ = alu_.calcular();
            break;
        case 0x03:
        case 0x23:
            alu_.write(a, d.imm, "ADD");
            aluOut_ = alu_.calcular();
            mar_ = std::bit_cast<std::uint32_t>(aluOut_);
            break;
        case 0x63:
            branchTaken_ = alu_.testBranchCondition(d.mnemonic, a, b);
            branchTarget_ = currentPC_ + static_cast<std::uint32_t>(d.imm);
            aluOut_ = branchTaken_ ? 1 : 0;
            break;
        case 0x37: aluOut_ = d.imm; break;
        case 0x17: aluOut_ = wrapAdd(std::bit_cast<std::int32_t>(currentPC_), d.imm); break;
        case 0x6f:
            aluOut_ = std::bit_cast<std::int32_t>(pcNext_);
            jumpTaken_ = true;
            jumpTarget_ = currentPC_ + static_cast<std::uint32_t>(d.imm);
            break;
        case 0x67:
            aluOut_ = std::bit_cast<std::int32_t>(pcNext_);
            jumpTaken_ = true;
            jumpTarget_ = (std::bit_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(d.imm)) & ~1U;
            break;
        case 0x73: aluOut_ = 0; break;
        default: aluOut_ = 0; break;
        }
        stageName_ = "EXECUTE";
        micro_ = "EX: ALU Result = " + std::to_string(aluOut_) + " (" +
                 toHex32(std::bit_cast<std::uint32_t>(aluOut_)) + ")";
        break;
    }
    case 4:
        stageName_ = "MEMORY";
        if (decoded_.opcode == 0x03) {
            switch (decoded_.funct3) {
            case 0: mdr_ = readMemoryByte(mar_, true); break;
            case 1: mdr_ = readMemoryHalf(mar_, true); break;
            case 2: mdr_ = readMemoryWord(mar_); break;
            case 4: mdr_ = readMemoryByte(mar_, false); break;
            case 5: mdr_ = readMemoryHalf(mar_, false); break;
            default: mdr_ = readMemoryWord(mar_); break;
            }
            mbr_ = std::bit_cast<std::uint32_t>(mdr_);
            micro_ = "MEM: Read Mem[" + toHex32(mar_) + "] -> " + toHex32(mbr_);
        } else if (decoded_.opcode == 0x23) {
            const auto value = std::bit_cast<std::uint32_t>(latB_);
            if (decoded_.funct3 == 0) writeMemoryByte(mar_, static_cast<std::uint8_t>(value));
            else if (decoded_.funct3 == 1) writeMemoryHalf(mar_, static_cast<std::uint16_t>(value));
            else if (decoded_.funct3 == 2) writeMemoryWord(mar_, value);
            mbr_ = value;
            micro_ = "MEM: Write Mem[" + toHex32(mar_) + "] <- " + toHex32(value);
        } else {
            micro_ = "MEM: N/A (" + decoded_.mnemonic + ")";
        }
        break;
    case 5:
        stageName_ = "WRITEBACK";
        if (decoded_.rd != 0) {
            if (decoded_.opcode == 0x03) regs_.write(decoded_.rd, mdr_);
            else if (decoded_.opcode == 0x33 || decoded_.opcode == 0x13 ||
                     decoded_.opcode == 0x37 || decoded_.opcode == 0x17 ||
                     decoded_.opcode == 0x6f || decoded_.opcode == 0x67)
                regs_.write(decoded_.rd, aluOut_);
        }
        if (branchTaken_) {
            regs_.writePC(branchTarget_);
            micro_ = "WB: Branch Tomado -> PC=" + toHex32(branchTarget_);
        } else if (jumpTaken_) {
            regs_.writePC(jumpTarget_);
            micro_ = "WB: Jump Tomado -> PC=" + toHex32(jumpTarget_);
        } else {
            regs_.writePC(pcNext_);
            micro_ = "WB: PC Avança -> PC=" + toHex32(pcNext_);
        }
        ++instrucoesConcluidas_;
        break;
    default: return false;
    }
    notify(sc, ciclos);
    return true;
}

std::uint64_t ControlUnit::getCompletedInstructions() const noexcept { return instrucoesConcluidas_; }

std::uint64_t ControlUnit::cacheHits() const noexcept { return 0; }
std::uint64_t ControlUnit::cacheMisses() const noexcept { return 0; }
std::uint64_t ControlUnit::cachePenaltyCycles() const noexcept { return 0; }

std::int32_t ControlUnit::readMemoryByte(std::uint32_t address, bool signedValue) {
    return ram_.readByte(address, signedValue);
}
std::int32_t ControlUnit::readMemoryHalf(std::uint32_t address, bool signedValue) {
    return ram_.readHalf(address, signedValue);
}
std::int32_t ControlUnit::readMemoryWord(std::uint32_t address) { return ram_.readWord(address); }
std::uint32_t ControlUnit::readMemoryWordUnsigned(std::uint32_t address) {
    return ram_.readWordUnsigned(address);
}
void ControlUnit::writeMemoryByte(std::uint32_t address, std::uint8_t value) {
    ram_.writeByte(address, value);
}
void ControlUnit::writeMemoryHalf(std::uint32_t address, std::uint16_t value) {
    ram_.writeHalf(address, value);
}
void ControlUnit::writeMemoryWord(std::uint32_t address, std::uint32_t value) {
    ram_.writeWord(address, value);
}

Estado ControlUnit::getEstado(std::uint8_t subciclo, std::uint64_t ciclos) const {
    Estado state;
    state.subciclo = subciclo;
    state.instrucoesConcluidas = instrucoesConcluidas_;
    state.ciclos = ciclos;
    state.stage = stageName_;
    state.mir = stageName_;
    state.mpc = decoded_.mnemonic;
    state.micro = micro_;
    state.pc = regs_.readPC();
    state.ir = ir_;
    state.mar = mar_;
    state.mbr = mbr_;
    state.latA = latA_;
    state.latB = latB_;
    state.aluRes = aluOut_;
    state.aluZ = alu_.zero();
    state.aluN = alu_.negative();
    state.branchTaken = branchTaken_;
    state.jumpTaken = jumpTaken_;
    state.decoded = decoded_;
    state.registers = regs_.readAll();
    state.registersList = regs_.getAllWithMeta();
    return state;
}

void ControlUnit::notify(std::uint8_t subciclo, std::uint64_t ciclos) const {
    if (onEstadoChange_) onEstadoChange_(getEstado(subciclo, ciclos));
}

void ControlUnit::hardReset(std::uint32_t baseAddress) {
    regs_.clear();
    regs_.writePC(baseAddress);
    currentPC_ = baseAddress;
    pcNext_ = baseAddress + 4U;
    mar_ = baseAddress;
    ir_ = ram_.readWordUnsigned(baseAddress);
    mbr_ = ir_;
    mdr_ = 0;
    latA_ = latB_ = aluOut_ = 0;
    stageName_ = "FETCH";
    decoded_ = decodeInstruction(ir_);
    micro_ = "Binário carregado em " + toHex32(baseAddress) + ". Instrução: " + decoded_.mnemonic;
    branchTaken_ = jumpTaken_ = false;
    instrucoesConcluidas_ = 0;
    notify(1, 0);
}

void ControlUnit::reset() {
    regs_.clear();
    ir_ = currentPC_ = pcNext_ = mar_ = mbr_ = 0;
    mdr_ = latA_ = latB_ = aluOut_ = 0;
    decoded_ = {};
    branchTaken_ = jumpTaken_ = false;
    branchTarget_ = jumpTarget_ = 0;
    instrucoesConcluidas_ = 0;
    stageName_ = "RESET";
    micro_ = "-";
    notify(1, 0);
}

Registers& ControlUnit::registradores() noexcept { return regs_; }
const Registers& ControlUnit::registradores() const noexcept { return regs_; }
Memory& ControlUnit::ram() noexcept { return ram_; }
const Memory& ControlUnit::ram() const noexcept { return ram_; }

} // namespace rv32i
