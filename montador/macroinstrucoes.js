// Gerador de Machine Code do RISC-V RV32I
// Implementa os 6 formatos da ISA RV32I:
// R-Type, I-Type, S-Type, B-Type, U-Type, J-Type

export const OPCODES = {
    // R-Type
    R_TYPE: 0b0110011, // 51 (0x33)

    // I-Type
    I_ALU:  0b0010011, // 19 (0x13) - addi, slli, slti, etc.
    I_LOAD: 0b0000011, // 3  (0x03) - lb, lh, lw, lbu, lhu
    I_JALR: 0b1100111, // 103 (0x67) - jalr
    I_SYS:  0b1110011, // 115 (0x73) - ecall, ebreak

    // S-Type
    S_TYPE: 0b0100011, // 35 (0x23) - sb, sh, sw

    // B-Type
    B_TYPE: 0b1100011, // 99 (0x63) - beq, bne, blt, bge, etc.

    // U-Type
    U_LUI:   0b0110111, // 55 (0x37) - lui
    U_AUIPC: 0b0010111, // 23 (0x17) - auipc

    // J-Type
    J_JAL:   0b1101111  // 111 (0x6F) - jal
};

export function encodeRType(funct7, rs2, rs1, funct3, rd, opcode) {
    const word =
        ((funct7 & 0x7F) << 25) |
        ((rs2 & 0x1F) << 20) |
        ((rs1 & 0x1F) << 15) |
        ((funct3 & 0x7) << 12) |
        ((rd & 0x1F) << 7) |
        (opcode & 0x7F);
    return (word >>> 0).toString(2).padStart(32, "0");
}

export function encodeIType(imm, rs1, funct3, rd, opcode) {
    const word =
        ((imm & 0xFFF) << 20) |
        ((rs1 & 0x1F) << 15) |
        ((funct3 & 0x7) << 12) |
        ((rd & 0x1F) << 7) |
        (opcode & 0x7F);
    return (word >>> 0).toString(2).padStart(32, "0");
}

export function encodeSType(imm, rs2, rs1, funct3, opcode) {
    const imm_11_5 = (imm >> 5) & 0x7F;
    const imm_4_0 = imm & 0x1F;
    const word =
        (imm_11_5 << 25) |
        ((rs2 & 0x1F) << 20) |
        ((rs1 & 0x1F) << 15) |
        ((funct3 & 0x7) << 12) |
        (imm_4_0 << 7) |
        (opcode & 0x7F);
    return (word >>> 0).toString(2).padStart(32, "0");
}

export function encodeBType(imm, rs2, rs1, funct3, opcode) {
    const imm12 = (imm >> 12) & 0x1;
    const imm11 = (imm >> 11) & 0x1;
    const imm10_5 = (imm >> 5) & 0x3F;
    const imm4_1 = (imm >> 1) & 0xF;
    const word =
        (imm12 << 31) |
        (imm10_5 << 25) |
        ((rs2 & 0x1F) << 20) |
        ((rs1 & 0x1F) << 15) |
        ((funct3 & 0x7) << 12) |
        (imm4_1 << 8) |
        (imm11 << 7) |
        (opcode & 0x7F);
    return (word >>> 0).toString(2).padStart(32, "0");
}

export function encodeUType(imm, rd, opcode) {
    // imm pode vir como valor de 20 bits ou 32 bits shifted
    const imm20 = imm > 0xFFFFF ? (imm >>> 12) & 0xFFFFF : imm & 0xFFFFF;
    const word =
        (imm20 << 12) |
        ((rd & 0x1F) << 7) |
        (opcode & 0x7F);
    return (word >>> 0).toString(2).padStart(32, "0");
}

export function encodeJType(imm, rd, opcode) {
    const imm20 = (imm >> 20) & 0x1;
    const imm10_1 = (imm >> 1) & 0x3FF;
    const imm11 = (imm >> 11) & 0x1;
    const imm19_12 = (imm >> 12) & 0xFF;
    const word =
        (imm20 << 31) |
        (imm10_1 << 21) |
        (imm11 << 20) |
        (imm19_12 << 12) |
        ((rd & 0x1F) << 7) |
        (opcode & 0x7F);
    return (word >>> 0).toString(2).padStart(32, "0");
}

export default function detecta_tipo_instrucao(linha) {
    if (!linha || !linha.opcode) return null;
    const op = linha.opcode.toUpperCase();
    const rd = linha.rd ?? 0;
    const rs1 = linha.rs1 ?? 0;
    const rs2 = linha.rs2 ?? 0;
    const imm = linha.imm ?? 0;

    switch (op) {
        // ── R-Type ───────────────────────────────
        case "ADD":  return encodeRType(0x00, rs2, rs1, 0x0, rd, OPCODES.R_TYPE);
        case "SUB":  return encodeRType(0x20, rs2, rs1, 0x0, rd, OPCODES.R_TYPE);
        case "SLL":  return encodeRType(0x00, rs2, rs1, 0x1, rd, OPCODES.R_TYPE);
        case "SLT":  return encodeRType(0x00, rs2, rs1, 0x2, rd, OPCODES.R_TYPE);
        case "SLTU": return encodeRType(0x00, rs2, rs1, 0x3, rd, OPCODES.R_TYPE);
        case "XOR":  return encodeRType(0x00, rs2, rs1, 0x4, rd, OPCODES.R_TYPE);
        case "SRL":  return encodeRType(0x00, rs2, rs1, 0x5, rd, OPCODES.R_TYPE);
        case "SRA":  return encodeRType(0x20, rs2, rs1, 0x5, rd, OPCODES.R_TYPE);
        case "OR":   return encodeRType(0x00, rs2, rs1, 0x6, rd, OPCODES.R_TYPE);
        case "AND":  return encodeRType(0x00, rs2, rs1, 0x7, rd, OPCODES.R_TYPE);

        // ── I-Type (Aritmética / Lógica) ─────────
        case "ADDI":  return encodeIType(imm, rs1, 0x0, rd, OPCODES.I_ALU);
        case "SLLI":  return encodeIType((0x00 << 5) | (imm & 0x1F), rs1, 0x1, rd, OPCODES.I_ALU);
        case "SLTI":  return encodeIType(imm, rs1, 0x2, rd, OPCODES.I_ALU);
        case "SLTIU": return encodeIType(imm, rs1, 0x3, rd, OPCODES.I_ALU);
        case "XORI":  return encodeIType(imm, rs1, 0x4, rd, OPCODES.I_ALU);
        case "SRLI":  return encodeIType((0x00 << 5) | (imm & 0x1F), rs1, 0x5, rd, OPCODES.I_ALU);
        case "SRAI":  return encodeIType((0x20 << 5) | (imm & 0x1F), rs1, 0x5, rd, OPCODES.I_ALU);
        case "ORI":   return encodeIType(imm, rs1, 0x6, rd, OPCODES.I_ALU);
        case "ANDI":  return encodeIType(imm, rs1, 0x7, rd, OPCODES.I_ALU);

        // ── I-Type (Loads) ───────────────────────
        case "LB":  return encodeIType(imm, rs1, 0x0, rd, OPCODES.I_LOAD);
        case "LH":  return encodeIType(imm, rs1, 0x1, rd, OPCODES.I_LOAD);
        case "LW":  return encodeIType(imm, rs1, 0x2, rd, OPCODES.I_LOAD);
        case "LBU": return encodeIType(imm, rs1, 0x4, rd, OPCODES.I_LOAD);
        case "LHU": return encodeIType(imm, rs1, 0x5, rd, OPCODES.I_LOAD);

        // ── I-Type (Jumps / Environment) ─────────
        case "JALR":   return encodeIType(imm, rs1, 0x0, rd, OPCODES.I_JALR);
        case "ECALL":  return encodeIType(0, 0, 0x0, 0, OPCODES.I_SYS);
        case "EBREAK": return encodeIType(1, 0, 0x0, 0, OPCODES.I_SYS);

        // ── S-Type (Stores) ──────────────────────
        case "SB": return encodeSType(imm, rs2, rs1, 0x0, OPCODES.S_TYPE);
        case "SH": return encodeSType(imm, rs2, rs1, 0x1, OPCODES.S_TYPE);
        case "SW": return encodeSType(imm, rs2, rs1, 0x2, OPCODES.S_TYPE);

        // ── B-Type (Branches) ────────────────────
        case "BEQ":  return encodeBType(imm, rs2, rs1, 0x0, OPCODES.B_TYPE);
        case "BNE":  return encodeBType(imm, rs2, rs1, 0x1, OPCODES.B_TYPE);
        case "BLT":  return encodeBType(imm, rs2, rs1, 0x4, OPCODES.B_TYPE);
        case "BGE":  return encodeBType(imm, rs2, rs1, 0x5, OPCODES.B_TYPE);
        case "BLTU": return encodeBType(imm, rs2, rs1, 0x6, OPCODES.B_TYPE);
        case "BGEU": return encodeBType(imm, rs2, rs1, 0x7, OPCODES.B_TYPE);

        // ── U-Type ───────────────────────────────
        case "LUI":   return encodeUType(imm, rd, OPCODES.U_LUI);
        case "AUIPC": return encodeUType(imm, rd, OPCODES.U_AUIPC);

        // ── J-Type ───────────────────────────────
        case "JAL": return encodeJType(imm, rd, OPCODES.J_JAL);

        default:
            console.error(`Instrução RV32I desconhecida: ${op}`);
            return null;
    }
}
