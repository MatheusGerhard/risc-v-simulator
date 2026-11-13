// ControlUnit.js - Unidade de Controle do Processador RISC-V RV32I
// Implementa uma FSM (Máquina de Estados Finitos) de Ciclo Múltiplo Clássica:
// 1: Fetch (IF) - Busca de Instrução
// 2: Decode (ID) - Decodificação e Leitura dos Registradores
// 3: Execute (EX) - Execução na ALU / Cálculo de Endereço / Desvios
// 4: Memory (MEM) - Acesso à Memória (Leitura/Escrita)
// 5: Write-back (WB) - Escrita nos Registradores de Destino

import ArithmeticLogicUnit from './componentes/alu.js';
import memoria from "./componentes/memory.js";
import Registers, { ABI_NAMES, toHex32, toBinary32 } from './componentes/registers.js';
import { logInstructionExecution } from '../src/services/simulationLog.js';

export class ControlUnit {
    constructor() {
        this.regs = new Registers();
        this.ram = memoria;
        this.alu = new ArithmeticLogicUnit();

        // Registradores de pipeline / temporários de microarquitetura
        this.ir = "00000000000000000000000000000000";
        this.irNum = 0;
        this.currentPC = 0;
        this.pcNext = 0;
        this.mar = 0;
        this.mbr = "00000000000000000000000000000000";
        this.mdr = 0;
        this.latA = 0;
        this.latB = 0;
        this.aluOut = 0;

        // Decodificação da instrução atual
        this.decoded = {
            opcode: 0,
            rd: 0,
            funct3: 0,
            rs1: 0,
            rs2: 0,
            funct7: 0,
            imm: 0,
            format: "UNKNOWN",
            mnemonic: "NOP"
        };

        // Flags e controles de salto
        this.branchTaken = false;
        this.branchTarget = 0;
        this.jumpTaken = false;
        this.instrucoesConcluidas = 0;
        this.jumpTarget = 0;

        // Estado visual
        this.stageName = "RESET";
        this.onEstadoChange = null;
        this.micro = "-";
        this.hits = 0;
        this.misses = 0;
        this.instrucoesConcluidas = 0;
    }

    setCallback(callback) {
        this.onEstadoChange = callback;
    }

    // Identifica e decodifica a instrução de 32 bits
    decodeInstruction(ir) {
        const opcode = ir & 0x7F;
        const rd = (ir >>> 7) & 0x1F;
        const funct3 = (ir >>> 12) & 0x07;
        const rs1 = (ir >>> 15) & 0x1F;
        const rs2 = (ir >>> 20) & 0x1F;
        const funct7 = (ir >>> 25) & 0x7F;

        // Imediatos com extensão de sinal
        const immI = (ir >> 20); // sign-extended 12 bits
        const rawS = ((ir >> 25) << 5) | ((ir >>> 7) & 0x1F);
        const immS = (rawS << 20) >> 20; // sign-extended 12 bits

        const b31 = (ir >>> 31) & 1;
        const b7 = (ir >>> 7) & 1;
        const b30_25 = (ir >>> 25) & 0x3F;
        const b11_8 = (ir >>> 8) & 0x0F;
        const rawB = (b31 << 12) | (b7 << 11) | (b30_25 << 5) | (b11_8 << 1);
        const immB = (rawB << 19) >> 19; // sign-extended 13 bits

        const immU = ir & 0xFFFFF000;

        const j31 = (ir >>> 31) & 1;
        const j19_12 = (ir >>> 12) & 0xFF;
        const j20 = (ir >>> 20) & 1;
        const j30_21 = (ir >>> 21) & 0x3FF;
        const rawJ = (j31 << 20) | (j19_12 << 12) | (j20 << 11) | (j30_21 << 1);
        const immJ = (rawJ << 11) >> 11; // sign-extended 21 bits

        let mnemonic = "UNKNOWN";
        let format = "UNKNOWN";
        let imm = 0;

        switch (opcode) {
            case 0x33: // R-Type
                format = "R-Type";
                imm = 0;
                if (funct3 === 0x0) mnemonic = funct7 === 0x20 ? "SUB" : "ADD";
                else if (funct3 === 0x1) mnemonic = "SLL";
                else if (funct3 === 0x2) mnemonic = "SLT";
                else if (funct3 === 0x3) mnemonic = "SLTU";
                else if (funct3 === 0x4) mnemonic = "XOR";
                else if (funct3 === 0x5) mnemonic = funct7 === 0x20 ? "SRA" : "SRL";
                else if (funct3 === 0x6) mnemonic = "OR";
                else if (funct3 === 0x7) mnemonic = "AND";
                break;

            case 0x13: // I-Type (Aritmética / Lógica imediata)
                format = "I-Type";
                imm = immI;
                if (funct3 === 0x0) mnemonic = "ADDI";
                else if (funct3 === 0x1) mnemonic = "SLLI";
                else if (funct3 === 0x2) mnemonic = "SLTI";
                else if (funct3 === 0x3) mnemonic = "SLTIU";
                else if (funct3 === 0x4) mnemonic = "XORI";
                else if (funct3 === 0x5) mnemonic = funct7 === 0x20 ? "SRAI" : "SRLI";
                else if (funct3 === 0x6) mnemonic = "ORI";
                else if (funct3 === 0x7) mnemonic = "ANDI";
                break;

            case 0x03: // I-Type (Loads)
                format = "I-Type";
                imm = immI;
                if (funct3 === 0x0) mnemonic = "LB";
                else if (funct3 === 0x1) mnemonic = "LH";
                else if (funct3 === 0x2) mnemonic = "LW";
                else if (funct3 === 0x4) mnemonic = "LBU";
                else if (funct3 === 0x5) mnemonic = "LHU";
                break;

            case 0x67: // I-Type (JALR)
                format = "I-Type";
                imm = immI;
                mnemonic = "JALR";
                break;

            case 0x23: // S-Type (Stores)
                format = "S-Type";
                imm = immS;
                if (funct3 === 0x0) mnemonic = "SB";
                else if (funct3 === 0x1) mnemonic = "SH";
                else if (funct3 === 0x2) mnemonic = "SW";
                break;

            case 0x63: // B-Type (Branches)
                format = "B-Type";
                imm = immB;
                if (funct3 === 0x0) mnemonic = "BEQ";
                else if (funct3 === 0x1) mnemonic = "BNE";
                else if (funct3 === 0x4) mnemonic = "BLT";
                else if (funct3 === 0x5) mnemonic = "BGE";
                else if (funct3 === 0x6) mnemonic = "BLTU";
                else if (funct3 === 0x7) mnemonic = "BGEU";
                break;

            case 0x37: // U-Type (LUI)
                format = "U-Type";
                imm = immU;
                mnemonic = "LUI";
                break;

            case 0x17: // U-Type (AUIPC)
                format = "U-Type";
                imm = immU;
                mnemonic = "AUIPC";
                break;

            case 0x6F: // J-Type (JAL)
                format = "J-Type";
                imm = immJ;
                mnemonic = "JAL";
                break;

            case 0x73: // System (ECALL / EBREAK)
                format = "I-Type";
                imm = immI;
                mnemonic = (immI === 1) ? "EBREAK" : "ECALL";
                break;

            default:
                if (ir === 0) {
                    mnemonic = "NOP / ZERO";
                }
                break;
        }

        return {
            opcode,
            rd,
            funct3,
            rs1,
            rs2,
            funct7,
            immI,
            immS,
            immB,
            immU,
            immJ,
            imm,
            format,
            mnemonic
        };
    }

    // Executa um subciclo da FSM de 5 estágios
    rodarCiclo(sc, ciclos) {
        switch (sc) {
            case 1: { // 1. FETCH (IF)
                this.currentPC = this.regs.readPC();
                this.mar = this.currentPC;

                // Busca a palavra de instrução de 32 bits
                this.irNum = this.ram.readWord(this.currentPC);
                this.ir = this.ram.readWordBinary(this.currentPC);
                this.mbr = this.ir;

                this.pcNext = (this.currentPC + 4) | 0;
                this.stageName = "FETCH";
                this.micro = `IF: PC=${toHex32(this.currentPC)} IR=${toHex32(this.irNum)}`;

                // Registra execução da instrução no log
                logInstructionExecution(this.currentPC, {
                    processor: "RV32I",
                    executionKey: `${ciclos}:${sc}`,
                    cycle: ciclos,
                    word: this.ir,
                });
                break;
            }

            case 2: { // 2. DECODE (ID)
                this.decoded = this.decodeInstruction(this.irNum);
                const d = this.decoded;

                // Leitura dos registradores fonte rs1 e rs2
                this.latA = this.regs.readInt(d.rs1);
                this.latB = this.regs.readInt(d.rs2);

                this.stageName = "DECODE";
                this.micro = `ID: ${d.mnemonic} (rd=x${d.rd}, rs1=x${d.rs1}, rs2=x${d.rs2}, imm=${d.imm})`;
                break;
            }

            case 3: { // 3. EXECUTE (EX)
                const d = this.decoded;
                const a = this.latA;
                const b = this.latB;
                const imm = d.imm;

                this.branchTaken = false;
                this.jumpTaken = false;

                switch (d.opcode) {
                    case 0x33: { // R-Type
                        this.alu.write(a, b, d.mnemonic);
                        this.aluOut = this.alu.calcular();
                        break;
                    }

                    case 0x13: { // I-Type Aritmética / Lógica
                        let op = "ADD";
                        if (d.funct3 === 0x0) op = "ADD";
                        else if (d.funct3 === 0x1) op = "SLL";
                        else if (d.funct3 === 0x2) op = "SLT";
                        else if (d.funct3 === 0x3) op = "SLTU";
                        else if (d.funct3 === 0x4) op = "XOR";
                        else if (d.funct3 === 0x5) op = (d.funct7 === 0x20) ? "SRA" : "SRL";
                        else if (d.funct3 === 0x6) op = "OR";
                        else if (d.funct3 === 0x7) op = "AND";

                        this.alu.write(a, imm, op);
                        this.aluOut = this.alu.calcular();
                        break;
                    }

                    case 0x03: { // Loads (LW, LH, LB, LHU, LBU)
                        // Cálculo de endereço de memória: rs1 + SignExt(imm)
                        this.alu.write(a, imm, "ADD");
                        this.aluOut = this.alu.calcular();
                        this.mar = this.aluOut;
                        break;
                    }

                    case 0x23: { // Stores (SW, SH, SB)
                        // Cálculo de endereço de memória: rs1 + SignExt(imm)
                        this.alu.write(a, imm, "ADD");
                        this.aluOut = this.alu.calcular();
                        this.mar = this.aluOut;
                        break;
                    }

                    case 0x63: { // B-Type (Branches)
                        this.branchTaken = this.alu.testBranchCondition(d.mnemonic, a, b);
                        this.branchTarget = (this.currentPC + imm) | 0;
                        this.aluOut = this.branchTaken ? 1 : 0;
                        break;
                    }

                    case 0x37: { // LUI (Load Upper Immediate)
                        this.aluOut = imm;
                        break;
                    }

                    case 0x17: { // AUIPC (Add Upper Immediate to PC)
                        this.aluOut = (this.currentPC + imm) | 0;
                        break;
                    }

                    case 0x6F: { // JAL (Jump and Link)
                        this.aluOut = (this.currentPC + 4) | 0; // rd = PC + 4
                        this.jumpTaken = true;
                        this.jumpTarget = (this.currentPC + imm) | 0;
                        break;
                    }

                    case 0x67: { // JALR (Jump and Link Register)
                        this.aluOut = (this.currentPC + 4) | 0; // rd = PC + 4
                        this.jumpTaken = true;
                        this.jumpTarget = ((a + imm) & ~1) | 0;
                        break;
                    }

                    case 0x73: { // ECALL / EBREAK
                        this.aluOut = 0;
                        break;
                    }

                    default:
                        this.aluOut = 0;
                        break;
                }

                this.stageName = "EXECUTE";
                this.micro = `EX: ALU Result = ${this.aluOut} (${toHex32(this.aluOut)})`;
                break;
            }

            case 4: { // 4. MEMORY (MEM)
                const d = this.decoded;
                this.stageName = "MEMORY";

                if (d.opcode === 0x03) { // Load
                    switch (d.funct3) {
                        case 0x0: // LB (signed byte)
                            this.mdr = this.ram.readByte(this.mar, true);
                            break;
                        case 0x1: // LH (signed half)
                            this.mdr = this.ram.readHalf(this.mar, true);
                            break;
                        case 0x2: // LW (signed word)
                            this.mdr = this.ram.readWord(this.mar);
                            break;
                        case 0x4: // LBU (unsigned byte)
                            this.mdr = this.ram.readByte(this.mar, false);
                            break;
                        case 0x5: // LHU (unsigned half)
                            this.mdr = this.ram.readHalf(this.mar, false);
                            break;
                        default:
                            this.mdr = this.ram.readWord(this.mar);
                    }
                    this.mbr = toBinary32(this.mdr);
                    this.micro = `MEM: Read Mem[${toHex32(this.mar)}] -> ${toHex32(this.mdr)}`;
                } else if (d.opcode === 0x23) { // Store
                    const valB = this.latB;
                    switch (d.funct3) {
                        case 0x0: // SB
                            this.ram.writeByte(this.mar, valB & 0xFF);
                            break;
                        case 0x1: // SH
                            this.ram.writeHalf(this.mar, valB & 0xFFFF);
                            break;
                        case 0x2: // SW
                            this.ram.writeWord(this.mar, valB);
                            break;
                    }
                    this.mbr = toBinary32(valB);
                    this.micro = `MEM: Write Mem[${toHex32(this.mar)}] <- ${toHex32(valB)}`;
                } else {
                    this.micro = `MEM: N/A (${d.mnemonic})`;
                }
                break;
            }

            case 5: { // 5. WRITE-BACK (WB)
                const d = this.decoded;
                this.stageName = "WRITEBACK";

                // Escrita no registrador de destino (rd)
                if (d.rd !== 0) {
                    if (d.opcode === 0x03) {
                        // Loads escrevem o dado lido da memória (MDR)
                        this.regs.write(d.rd, this.mdr);
                    } else if (
                        d.opcode === 0x33 || // R-Type
                        d.opcode === 0x13 || // I-Type
                        d.opcode === 0x37 || // LUI
                        d.opcode === 0x17 || // AUIPC
                        d.opcode === 0x6F || // JAL
                        d.opcode === 0x67    // JALR
                    ) {
                        this.regs.write(d.rd, this.aluOut);
                    }
                }

                // Atualização do PC
                if (this.branchTaken) {
                    this.regs.writePC(this.branchTarget);
                    this.micro = `WB: Branch Tomado -> PC=${toHex32(this.branchTarget)}`;
                } else if (this.jumpTaken) {
                    this.regs.writePC(this.jumpTarget);
                    this.micro = `WB: Jump Tomado -> PC=${toHex32(this.jumpTarget)}`;
                } else {
                    this.regs.writePC(this.pcNext);
                    this.micro = `WB: PC Avança -> PC=${toHex32(this.pcNext)}`;
                }

                // Incrementa contador de instruções concluídas (para cálculo de IPS)
                this.instrucoesConcluidas++;
                break;
            }

            default:
                break;
        }

        if (this.onEstadoChange) {
            this.onEstadoChange(this.getEstado(sc, ciclos));
        }

        return true;
    }

    getCompletedInstructions() {
        return this.instrucoesConcluidas;
    }

    // Hard Reset: Zera todos os registradores, limpa flags e aponta PC para baseAddress
    hardReset(baseAddress = 0) {
        this.regs.clear();
        const startAddr = typeof baseAddress === "string"
            ? (baseAddress.startsWith("0x") || baseAddress.startsWith("0X") ? parseInt(baseAddress, 16) : parseInt(baseAddress, 10))
            : Number(baseAddress);

        this.regs.writePC(startAddr);
        this.currentPC = startAddr;
        this.pcNext = startAddr + 4;
        this.mar = startAddr;

        // Pré-carrega e decodifica a instrução inicial no endereço base para atualização imediata da tela
        this.irNum = this.ram.readWord(startAddr);
        this.ir = this.ram.readWordBinary(startAddr);
        this.mbr = this.ir;

        this.mdr = 0;
        this.latA = 0;
        this.latB = 0;
        this.aluOut = 0;
        this.stageName = "FETCH";
        this.decoded = this.decodeInstruction(this.irNum);
        this.micro = `Binário carregado em ${toHex32(startAddr)}. Instrução: ${this.decoded.mnemonic}`;
        this.branchTaken = false;
        this.jumpTaken = false;
        this.instrucoesConcluidas = 0;

        if (this.onEstadoChange) {
            this.onEstadoChange(this.getEstado(1, 0));
        }
    }

    reset() {
        this.regs.clear();
        this.ir = "00000000000000000000000000000000";
        this.irNum = 0;
        this.currentPC = 0;
        this.pcNext = 0;
        this.mar = 0;
        this.mbr = "00000000000000000000000000000000";
        this.mdr = 0;
        this.latA = 0;
        this.latB = 0;
        this.aluOut = 0;
        this.stageName = "RESET";
        this.micro = "-";
        this.decoded = {
            opcode: 0,
            rd: 0,
            funct3: 0,
            rs1: 0,
            rs2: 0,
            funct7: 0,
            imm: 0,
            format: "UNKNOWN",
            mnemonic: "NOP"
        };
        this.branchTaken = false;
        this.jumpTaken = false;
        this.instrucoesConcluidas = 0;

        if (this.onEstadoChange) {
            this.onEstadoChange(this.getEstado(1, 0));
        }
    }

    // Monta snapshot completo do processador RV32I para a interface React
    getEstado(sc = 1, ciclos = 0) {
        const pcInt = this.regs.readPC();
        const registersWithMeta = this.regs.getAllWithMeta();

        // Mapeamento dinâmico dos registradores para fácil acesso
        const regMap = {};
        for (let i = 0; i < 32; i++) {
            regMap[`x${i}`] = this.regs.read(i);
            regMap[ABI_NAMES[i]] = this.regs.read(i);
        }

        return {
            subciclo: sc,
            instrucoesConcluidas: this.instrucoesConcluidas,
            ciclos: ciclos,
            stage: this.stageName,
            mir: this.stageName,
            mpc: this.decoded.mnemonic,
            micro: this.micro,

            // Barramentos e Registradores Especiais
            pc: toHex32(pcInt),
            pcDec: pcInt,
            pcBin: toBinary32(pcInt),

            ir: this.ir,
            irHex: toHex32(this.irNum),
            mar: toHex32(this.mar),
            marDec: this.mar,
            mbr: this.mbr,
            mbrHex: toHex32(this.mdr),

            // Sinais do Caminho de Dados
            latA: toHex32(this.latA),
            latADec: this.latA,
            latB: toHex32(this.latB),
            latBDec: this.latB,
            amux: toHex32(this.latA),
            aluRes: toHex32(this.aluOut),
            aluResDec: this.aluOut,
            aluZ: this.alu.Z,
            aluN: this.alu.N,
            shifter: toHex32(this.aluOut),
            mmux: this.branchTaken || this.jumpTaken ? "BRANCH/JUMP" : "PC+4",
            msl: this.branchTaken ? "TAKEN" : "NOT TAKEN",

            // Decodificação
            opcodeName: this.decoded.mnemonic,
            format: this.decoded.format,
            rd: `x${this.decoded.rd} (${ABI_NAMES[this.decoded.rd]})`,
            rs1: `x${this.decoded.rs1} (${ABI_NAMES[this.decoded.rs1]})`,
            rs2: `x${this.decoded.rs2} (${ABI_NAMES[this.decoded.rs2]})`,
            imm: toHex32(this.decoded.imm),
            immDec: this.decoded.imm,

            // Lista rica de 32 registradores
            registersList: registersWithMeta,

            // Compatibilidade retroativa para elementos do MIC-1
            ac: toHex32(this.regs.readInt(10)), // a0
            sp: toHex32(this.regs.readInt(2)),  // sp
            tir: this.ir,
            zero: "00000000000000000000000000000000",
            um: "00000000000000000000000000000001",
            menosUm: "11111111111111111111111111111111",
            am: "00000000000000000000111111111111",
            sm: "00000000000000000000000011111111",
            regA: toHex32(this.regs.readInt(10)),
            regB: toHex32(this.regs.readInt(11)),
            regC: toHex32(this.regs.readInt(12)),
            regD: toHex32(this.regs.readInt(13)),
            regE: toHex32(this.regs.readInt(14)),
            regF: toHex32(this.regs.readInt(15)),
            decA: `x${this.decoded.rs1}`,
            decB: `x${this.decoded.rs2}`,
            decC: `x${this.decoded.rd}`,

            ...regMap
        };
    }
}

export default ControlUnit;
