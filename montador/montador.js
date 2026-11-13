// Montador (Assembler) do RISC-V RV32I
// Realiza montagem em duas passagens:
// 1ª Passagem: Análise léxica, expansão de pseudoinstruções, cálculo de endereços (PC = i * 4) e mapeamento de rótulos (labels).
// 2ª Passagem: Resolução de labels e offsets relativos ao PC, codificação dos campos (rd, rs1, rs2, imm, funct3, funct7, opcode) e geração do código de máquina de 32 bits.

import detecta_tipo_instrucao from "./macroinstrucoes.js";
import memoria from "../mac1/componentes/memory.js";
import { parseRegisterName } from "../mac1/componentes/registers.js";
import { registerMacroInstructions } from "../src/services/simulationLog.js";

export class Montador {
    constructor(assemblyCode) {
        this.rawCode = assemblyCode || "";
        this.macroinstrucoes = [];
        this.microinstrucoes = [];
        this.labelTable = new Map();
    }

    // Normaliza número imediato (dec, hex, bin)
    parseImmediate(token) {
        if (typeof token === "number") return token;
        if (!token) return 0;
        const clean = token.trim();
        if (clean.startsWith("0x") || clean.startsWith("0X") || clean.startsWith("-0x") || clean.startsWith("-0X")) {
            const isNeg = clean.startsWith("-");
            const hexPart = isNeg ? clean.slice(3) : clean.slice(2);
            const val = parseInt(hexPart, 16);
            return isNeg ? -val : val;
        }
        if (clean.startsWith("0b") || clean.startsWith("0B") || clean.startsWith("-0b") || clean.startsWith("-0B")) {
            const isNeg = clean.startsWith("-");
            const binPart = isNeg ? clean.slice(3) : clean.slice(2);
            const val = parseInt(binPart, 2);
            return isNeg ? -val : val;
        }
        return parseInt(clean, 10);
    }

    // Separa operandos respeitando vírgulas e parênteses de memória como offset(rs1)
    splitOperands(operandsStr) {
        if (!operandsStr) return [];
        return operandsStr
            .split(",")
            .map(s => s.trim())
            .filter(s => s.length > 0);
    }

    // Processa operando de memória como "4(sp)" ou "-8(s0)" ou "0(x1)"
    parseMemoryOperand(operand) {
        const match = operand.match(/^([+-]?\w+)\s*\(([^)]+)\)$/);
        if (match) {
            return {
                offset: match[1],
                baseReg: match[2]
            };
        }
        return null;
    }

    // Expande pseudoinstruções para instruções base do RV32I
    expandPseudoInstruction(opcode, operands) {
        const op = opcode.toUpperCase();

        switch (op) {
            case "NOP":
                return [{ opcode: "ADDI", operands: ["x0", "x0", "0"] }];

            case "MV": // mv rd, rs1 -> addi rd, rs1, 0
                return [{ opcode: "ADDI", operands: [operands[0], operands[1], "0"] }];

            case "NOT": // not rd, rs1 -> xori rd, rs1, -1
                return [{ opcode: "XORI", operands: [operands[0], operands[1], "-1"] }];

            case "NEG": // neg rd, rs1 -> sub rd, x0, rs1
                return [{ opcode: "SUB", operands: [operands[0], "x0", operands[1]] }];

            case "SEQZ": // seqz rd, rs1 -> sltiu rd, rs1, 1
                return [{ opcode: "SLTIU", operands: [operands[0], operands[1], "1"] }];

            case "SNEZ": // snez rd, rs1 -> sltu rd, x0, rs1
                return [{ opcode: "SLTU", operands: [operands[0], "x0", operands[1]] }];

            case "SLTZ": // sltz rd, rs1 -> slt rd, rs1, x0
                return [{ opcode: "SLT", operands: [operands[0], operands[1], "x0"] }];

            case "SGTZ": // sgtz rd, rs1 -> slt rd, x0, rs1
                return [{ opcode: "SLT", operands: [operands[0], "x0", operands[1]] }];

            case "BEQZ": // beqz rs1, label -> beq rs1, x0, label
                return [{ opcode: "BEQ", operands: [operands[0], "x0", operands[1]] }];

            case "BNEZ": // bnez rs1, label -> bne rs1, x0, label
                return [{ opcode: "BNE", operands: [operands[0], "x0", operands[1]] }];

            case "BLEZ": // blez rs1, label -> bge x0, rs1, label
                return [{ opcode: "BGE", operands: ["x0", operands[0], operands[1]] }];

            case "BGEZ": // bgez rs1, label -> bge rs1, x0, label
                return [{ opcode: "BGE", operands: [operands[0], "x0", operands[1]] }];

            case "BLTZ": // bltz rs1, label -> blt rs1, x0, label
                return [{ opcode: "BLT", operands: [operands[0], "x0", operands[1]] }];

            case "BGTZ": // bgtz rs1, label -> blt x0, rs1, label
                return [{ opcode: "BLT", operands: ["x0", operands[0], operands[1]] }];

            case "BGT": // bgt rs1, rs2, label -> blt rs2, rs1, label
                return [{ opcode: "BLT", operands: [operands[1], operands[0], operands[2]] }];

            case "BLE": // ble rs1, rs2, label -> bge rs2, rs1, label
                return [{ opcode: "BGE", operands: [operands[1], operands[0], operands[2]] }];

            case "BGTU": // bgtu rs1, rs2, label -> bltu rs2, rs1, label
                return [{ opcode: "BLTU", operands: [operands[1], operands[0], operands[2]] }];

            case "BLEU": // bleu rs1, rs2, label -> bgeu rs2, rs1, label
                return [{ opcode: "BGEU", operands: [operands[1], operands[0], operands[2]] }];

            case "J": // j label -> jal x0, label
                return [{ opcode: "JAL", operands: ["x0", operands[0]] }];

            case "JR": // jr rs1 -> jalr x0, rs1, 0
                return [{ opcode: "JALR", operands: ["x0", operands[0], "0"] }];

            case "RET": // ret -> jalr x0, ra, 0
                return [{ opcode: "JALR", operands: ["x0", "ra", "0"] }];

            case "LI": { // li rd, imm
                const rd = operands[0];
                const immVal = this.parseImmediate(operands[1]);
                if (immVal >= -2048 && immVal <= 2047) {
                    return [{ opcode: "ADDI", operands: [rd, "x0", String(immVal)] }];
                } else {
                    // Para 32 bits: lui rd, upper + addi rd, rd, lower
                    let upper = (immVal + 0x800) >>> 12;
                    let lower = (immVal - (upper << 12)) | 0;
                    return [
                        { opcode: "LUI", operands: [rd, String(upper)] },
                        { opcode: "ADDI", operands: [rd, rd, String(lower)] }
                    ];
                }
            }

            default:
                return [{ opcode: op, operands }];
        }
    }

    // ─────────────────────────────────────────────
    // PRIMEIRA PASSAGEM
    // ─────────────────────────────────────────────
    primeira_passagem() {
        const lines = this.rawCode.split("\n");
        const parsedInstructions = [];
        this.labelTable.clear();

        let currentByteAddress = 0;

        for (let lineIndex = 0; lineIndex < lines.length; lineIndex++) {
            let line = lines[lineIndex];

            // Remove comentários iniciados com ';' ou '#'
            const commentIndex = line.search(/[;#]/);
            if (commentIndex !== -1) {
                line = line.substring(0, commentIndex);
            }
            line = line.trim();
            if (line.length === 0) continue;

            // Identifica rótulo (ex: "loop:" ou "inicio: addi x1, x1, 1")
            let label = null;
            const colonIndex = line.indexOf(":");
            if (colonIndex !== -1) {
                label = line.substring(0, colonIndex).trim();
                line = line.substring(colonIndex + 1).trim();
                if (label.length > 0) {
                    this.labelTable.set(label.toUpperCase(), currentByteAddress);
                }
            }

            if (line.length === 0) {
                // Linha continha apenas um rótulo
                continue;
            }

            // Separa o mnemônico dos operandos
            const firstSpace = line.search(/\s/);
            let opcode = "";
            let operandsStr = "";

            if (firstSpace === -1) {
                opcode = line.trim().toUpperCase();
            } else {
                opcode = line.substring(0, firstSpace).trim().toUpperCase();
                operandsStr = line.substring(firstSpace).trim();
            }

            const operands = this.splitOperands(operandsStr);

            // Expande pseudoinstruções
            const expanded = this.expandPseudoInstruction(opcode, operands);

            for (const item of expanded) {
                parsedInstructions.push({
                    originalLine: line,
                    byteAddress: currentByteAddress,
                    rotulo_index: Math.floor(currentByteAddress / 4),
                    opcode: item.opcode,
                    operands: item.operands,
                    labelRef: null,
                    rd: 0,
                    rs1: 0,
                    rs2: 0,
                    imm: 0
                });
                currentByteAddress += 4;
            }
        }

        this.macroinstrucoes = parsedInstructions;
        return parsedInstructions;
    }

    // ─────────────────────────────────────────────
    // SEGUNDA PASSAGEM
    // ─────────────────────────────────────────────
    segunda_passagem() {
        this.microinstrucoes = [];

        for (let i = 0; i < this.macroinstrucoes.length; i++) {
            const instr = this.macroinstrucoes[i];
            const op = instr.opcode;
            const ops = instr.operands;
            const pc = instr.byteAddress;

            try {
                switch (op) {
                    // ── R-Type: OP rd, rs1, rs2 ───────────────
                    case "ADD":
                    case "SUB":
                    case "SLL":
                    case "SLT":
                    case "SLTU":
                    case "XOR":
                    case "SRL":
                    case "SRA":
                    case "OR":
                    case "AND": {
                        if (ops.length < 3) throw new Error(`${op} requer 3 registradores: rd, rs1, rs2`);
                        instr.rd = parseRegisterName(ops[0]);
                        instr.rs1 = parseRegisterName(ops[1]);
                        instr.rs2 = parseRegisterName(ops[2]);
                        break;
                    }

                    // ── I-Type ALU: OP rd, rs1, imm ──────────
                    case "ADDI":
                    case "SLLI":
                    case "SLTI":
                    case "SLTIU":
                    case "XORI":
                    case "SRLI":
                    case "SRAI":
                    case "ORI":
                    case "ANDI": {
                        if (ops.length < 3) throw new Error(`${op} requer: rd, rs1, imm`);
                        instr.rd = parseRegisterName(ops[0]);
                        instr.rs1 = parseRegisterName(ops[1]);
                        instr.imm = this.parseImmediate(ops[2]);
                        break;
                    }

                    // ── I-Type Loads: OP rd, imm(rs1) ────────
                    case "LB":
                    case "LH":
                    case "LW":
                    case "LBU":
                    case "LHU": {
                        instr.rd = parseRegisterName(ops[0]);
                        if (ops.length === 2) {
                            const mem = this.parseMemoryOperand(ops[1]);
                            if (mem) {
                                instr.imm = this.parseImmediate(mem.offset);
                                instr.rs1 = parseRegisterName(mem.baseReg);
                            } else {
                                throw new Error(`Formato de memória inválido para ${op}: ${ops[1]}`);
                            }
                        } else if (ops.length === 3) {
                            instr.rs1 = parseRegisterName(ops[1]);
                            instr.imm = this.parseImmediate(ops[2]);
                        }
                        break;
                    }

                    // ── S-Type Stores: OP rs2, imm(rs1) ──────
                    case "SB":
                    case "SH":
                    case "SW": {
                        instr.rs2 = parseRegisterName(ops[0]);
                        if (ops.length === 2) {
                            const mem = this.parseMemoryOperand(ops[1]);
                            if (mem) {
                                instr.imm = this.parseImmediate(mem.offset);
                                instr.rs1 = parseRegisterName(mem.baseReg);
                            } else {
                                throw new Error(`Formato de memória inválido para ${op}: ${ops[1]}`);
                            }
                        } else if (ops.length === 3) {
                            instr.rs1 = parseRegisterName(ops[1]);
                            instr.imm = this.parseImmediate(ops[2]);
                        }
                        break;
                    }

                    // ── B-Type Branches: OP rs1, rs2, label/offset
                    case "BEQ":
                    case "BNE":
                    case "BLT":
                    case "BGE":
                    case "BLTU":
                    case "BGEU": {
                        if (ops.length < 3) throw new Error(`${op} requer: rs1, rs2, label`);
                        instr.rs1 = parseRegisterName(ops[0]);
                        instr.rs2 = parseRegisterName(ops[1]);

                        const targetStr = ops[2].toUpperCase();
                        if (this.labelTable.has(targetStr)) {
                            const targetAddress = this.labelTable.get(targetStr);
                            instr.imm = targetAddress - pc;
                            instr.labelRef = targetStr;
                        } else {
                            instr.imm = this.parseImmediate(ops[2]);
                        }
                        break;
                    }

                    // ── U-Type: OP rd, imm ───────────────────
                    case "LUI":
                    case "AUIPC": {
                        if (ops.length < 2) throw new Error(`${op} requer: rd, imm`);
                        instr.rd = parseRegisterName(ops[0]);
                        instr.imm = this.parseImmediate(ops[1]);
                        break;
                    }

                    // ── J-Type: JAL rd, label (ou JAL label) ──
                    case "JAL": {
                        let targetStr = "";
                        if (ops.length === 1) {
                            instr.rd = 1; // default: ra (x1)
                            targetStr = ops[0].toUpperCase();
                        } else {
                            instr.rd = parseRegisterName(ops[0]);
                            targetStr = ops[1].toUpperCase();
                        }

                        if (this.labelTable.has(targetStr)) {
                            const targetAddress = this.labelTable.get(targetStr);
                            instr.imm = targetAddress - pc;
                            instr.labelRef = targetStr;
                        } else {
                            instr.imm = this.parseImmediate(targetStr);
                        }
                        break;
                    }

                    // ── I-Type JALR: JALR rd, rs1, imm ───────
                    case "JALR": {
                        if (ops.length === 1) {
                            instr.rd = 1; // ra
                            instr.rs1 = parseRegisterName(ops[0]);
                            instr.imm = 0;
                        } else if (ops.length === 2) {
                            instr.rd = parseRegisterName(ops[0]);
                            const mem = this.parseMemoryOperand(ops[1]);
                            if (mem) {
                                instr.imm = this.parseImmediate(mem.offset);
                                instr.rs1 = parseRegisterName(mem.baseReg);
                            } else {
                                instr.rs1 = parseRegisterName(ops[1]);
                                instr.imm = 0;
                            }
                        } else if (ops.length === 3) {
                            instr.rd = parseRegisterName(ops[0]);
                            instr.rs1 = parseRegisterName(ops[1]);
                            instr.imm = this.parseImmediate(ops[2]);
                        }
                        break;
                    }

                    // ── Environment ─────────────────────────
                    case "ECALL":
                    case "EBREAK": {
                        instr.rd = 0;
                        instr.rs1 = 0;
                        instr.imm = op === "EBREAK" ? 1 : 0;
                        break;
                    }

                    default:
                        throw new Error(`Instrução não reconhecida: ${op}`);
                }

                const binWord = detecta_tipo_instrucao(instr);
                if (!binWord) {
                    throw new Error(`Falha ao codificar instrução: ${op}`);
                }
                this.microinstrucoes.push(binWord);
            } catch (err) {
                console.error(`Erro ao montar instrução ${i} (PC=${pc}):`, err.message);
                return { error: `Erro na instrução '${instr.originalLine}' (PC ${pc}): ${err.message}` };
            }
        }

        return true;
    }

    // Grava as instruções codificadas na memória e registra no catálogo de logs
    preencherMemoria() {
        registerMacroInstructions(this.macroinstrucoes);
        memoria.preencheInstrucoes(this.microinstrucoes, this.macroinstrucoes);
    }

    main() {
        const pass1 = this.primeira_passagem();
        if (pass1.error) {
            return pass1.error;
        }

        const pass2 = this.segunda_passagem();
        if (pass2 && pass2.error) {
            return pass2.error;
        }

        return false; // Retorna false indicando sucesso (padrão do projeto original)
    }
}

export default Montador;
