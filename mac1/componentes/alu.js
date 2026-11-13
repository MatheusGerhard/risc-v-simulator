// Arithmetic Logic Unit - Unidade Lógica e Aritmética (ALU) do RISC-V RV32I
// Implementa as operações inteiras de 32 bits:
// ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
// Gera flags Z (Zero) e N (Negativo) e suporta avaliação de desvios condicionais

export class ArithmeticLogicUnit {
    constructor() {
        this.a = 0;       // Operando A (32-bit int)
        this.b = 0;       // Operando B (32-bit int)
        this.op = "ADD";  // Operação
        this.res = "00000000000000000000000000000000";
        this.resInt = 0;
        this.Z = 1;       // 1 se o resultado for zero
        this.N = 0;       // 1 se o resultado for negativo (bit 31 = 1)
    }

    // Normaliza operando de entrada (aceita número ou string binária/hex)
    normalizeInput(val) {
        if (typeof val === "number") return val | 0;
        if (typeof val === "string") {
            if (val.startsWith("0x") || val.startsWith("0X")) {
                return parseInt(val, 16) | 0;
            }
            if (/^[01]+$/.test(val)) {
                return parseInt(val, 2) | 0;
            }
            return parseInt(val, 10) | 0;
        }
        return 0;
    }

    // Recebe operandos e operação
    write(a, b, op = "ADD") {
        this.a = this.normalizeInput(a);
        this.b = this.normalizeInput(b);
        this.op = String(op).toUpperCase().trim();
    }

    // Executa a operação configurada
    calcular() {
        const a = this.a;
        const b = this.b;
        const shamt = b & 0x1F; // RISC-V shifts usam apenas os 5 bits inferiores
        let result = 0;

        switch (this.op) {
            case "ADD":
            case "00": // compatibilidade com códigos antigos
                result = (a + b) | 0;
                break;

            case "SUB":
                result = (a - b) | 0;
                break;

            case "SLL": // Shift Left Logical
                result = (a << shamt) | 0;
                break;

            case "SLT": // Set Less Than (signed)
                result = (a < b) ? 1 : 0;
                break;

            case "SLTU": // Set Less Than Unsigned
                result = ((a >>> 0) < (b >>> 0)) ? 1 : 0;
                break;

            case "XOR":
                result = (a ^ b) | 0;
                break;

            case "SRL": // Shift Right Logical
                result = (a >>> shamt) | 0;
                break;

            case "SRA": // Shift Right Arithmetic
                result = (a >> shamt) | 0;
                break;

            case "OR":
                result = (a | b) | 0;
                break;

            case "AND":
            case "01": // compatibilidade
                result = (a & b) | 0;
                break;

            case "PASS_A":
            case "10": // compatibilidade
                result = a;
                break;

            case "NOT_A":
            case "11": // compatibilidade
                result = (~a) | 0;
                break;

            default:
                result = (a + b) | 0;
                break;
        }

        this.resInt = result | 0;
        this.res = (this.resInt >>> 0).toString(2).padStart(32, "0");
        this.Z = (this.resInt === 0) ? 1 : 0;
        this.N = (this.resInt < 0) ? 1 : 0;

        return this.resInt;
    }

    // Testa condição para instruções de Branch (B-Type)
    testBranchCondition(branchOp, valA, valB) {
        const a = this.normalizeInput(valA);
        const b = this.normalizeInput(valB);

        switch (branchOp.toUpperCase()) {
            case "BEQ":
                return a === b;
            case "BNE":
                return a !== b;
            case "BLT":
                return a < b; // signed
            case "BGE":
                return a >= b; // signed
            case "BLTU":
                return (a >>> 0) < (b >>> 0); // unsigned
            case "BGEU":
                return (a >>> 0) >= (b >>> 0); // unsigned
            default:
                return false;
        }
    }

    // Leitura compatível
    read(field) {
        if (field === "res") return this.res;
        if (field === "resInt") return this.resInt;
        if (field === "Z") return this.Z;
        if (field === "N") return this.N;
        return this[field];
    }
}

export default ArithmeticLogicUnit;
