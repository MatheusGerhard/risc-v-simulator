// Banco de Registradores do RISC-V RV32I
// 32 registradores de propósito geral (x0 a x31) + PC (Program Counter)
// x0 é hardwired para 0 (leitura sempre retorna 0, escritas são ignoradas)

export const ABI_NAMES = [
    "zero", // x0:  Constante 0
    "ra",   // x1:  Return address
    "sp",   // x2:  Stack pointer
    "gp",   // x3:  Global pointer
    "tp",   // x4:  Thread pointer
    "t0",   // x5:  Temporary 0
    "t1",   // x6:  Temporary 1
    "t2",   // x7:  Temporary 2
    "s0",   // x8:  Saved register 0 / Frame pointer (fp)
    "s1",   // x9:  Saved register 1
    "a0",   // x10: Function argument 0 / Return value 0
    "a1",   // x11: Function argument 1 / Return value 1
    "a2",   // x12: Function argument 2
    "a3",   // x13: Function argument 3
    "a4",   // x14: Function argument 4
    "a5",   // x15: Function argument 5
    "a6",   // x16: Function argument 6
    "a7",   // x17: Function argument 7
    "s2",   // x18: Saved register 2
    "s3",   // x19: Saved register 3
    "s4",   // x20: Saved register 4
    "s5",   // x21: Saved register 5
    "s6",   // x22: Saved register 6
    "s7",   // x23: Saved register 7
    "s8",   // x24: Saved register 8
    "s9",   // x25: Saved register 9
    "s10",  // x26: Saved register 10
    "s11",  // x27: Saved register 11
    "t3",   // x28: Temporary 3
    "t4",   // x29: Temporary 4
    "t5",   // x30: Temporary 5
    "t6"    // x31: Temporary 6
];

export const ABI_MAP = {
    zero: 0,
    ra: 1,
    sp: 2,
    gp: 3,
    tp: 4,
    t0: 5,
    t1: 6,
    t2: 7,
    s0: 8, fp: 8,
    s1: 9,
    a0: 10,
    a1: 11,
    a2: 12,
    a3: 13,
    a4: 14,
    a5: 15,
    a6: 16,
    a7: 17,
    s2: 18,
    s3: 19,
    s4: 20,
    s5: 21,
    s6: 22,
    s7: 23,
    s8: 24,
    s9: 25,
    s10: 26,
    s11: 27,
    t3: 28,
    t4: 29,
    t5: 30,
    t6: 31
};

export function parseRegisterName(regStr) {
    if (typeof regStr !== "string") return null;
    const clean = regStr.trim().toLowerCase().replace(",", "");
    if (/^x([0-9]|[12][0-9]|3[01])$/.test(clean)) {
        return parseInt(clean.slice(1), 10);
    }
    if (clean in ABI_MAP) {
        return ABI_MAP[clean];
    }
    return null;
}

export function toBinary32(val) {
    const num = typeof val === "string" ? (val.startsWith("0b") ? parseInt(val.slice(2), 2) : (val.startsWith("0x") ? parseInt(val, 16) : parseInt(val, 2))) : Number(val);
    return (num >>> 0).toString(2).padStart(32, "0");
}

export function toHex32(val) {
    const num = typeof val === "string" ? (val.startsWith("0x") ? parseInt(val, 16) : parseInt(val, 2)) : Number(val);
    return "0x" + (num >>> 0).toString(16).padStart(8, "0").toUpperCase();
}

class Registers {
    constructor() {
        // 32 registradores de 32 bits (armazenados como inteiros de 32 bits)
        this.data = new Int32Array(32);
        this.pc = 0;
    }

    // Escreve um valor em um registrador específico (x0 nunca é modificado)
    write(regNum, newValue) {
        const index = typeof regNum === "string" ? parseRegisterName(regNum) : regNum;
        if (index === null || index === undefined || index <= 0 || index > 31) {
            return; // x0 é somente leitura (sempre 0) e índices inválidos são ignorados
        }

        let intVal = 0;
        if (typeof newValue === "string") {
            if (newValue.startsWith("0x") || newValue.startsWith("0X")) {
                intVal = parseInt(newValue, 16) | 0;
            } else if (newValue.length === 32 || /^[01]+$/.test(newValue)) {
                intVal = parseInt(newValue, 2) | 0;
            } else {
                intVal = parseInt(newValue, 10) | 0;
            }
        } else {
            intVal = Number(newValue) | 0;
        }

        this.data[index] = intVal;
    }

    // Leitura no formato binário de 32 bits (padrão do simulador)
    read(regNum) {
        const index = typeof regNum === "string" ? parseRegisterName(regNum) : regNum;
        if (index === null || index === undefined || index < 0 || index > 31) {
            return "00000000000000000000000000000000";
        }
        if (index === 0) {
            return "00000000000000000000000000000000";
        }
        return (this.data[index] >>> 0).toString(2).padStart(32, "0");
    }

    // Leitura como inteiro com sinal (Int32)
    readInt(regNum) {
        const index = typeof regNum === "string" ? parseRegisterName(regNum) : regNum;
        if (index === null || index === undefined || index <= 0 || index > 31) {
            return 0;
        }
        return this.data[index] | 0;
    }

    // Leitura como inteiro sem sinal (UInt32)
    readUInt(regNum) {
        return (this.readInt(regNum) >>> 0);
    }

    // Leitura como string Hexadecimal
    readHex(regNum) {
        return toHex32(this.readUInt(regNum));
    }

    // Leitura e escrita do PC
    readPC() {
        return this.pc | 0;
    }

    readPCBinary() {
        return (this.pc >>> 0).toString(2).padStart(32, "0");
    }

    readPCHex() {
        return toHex32(this.pc);
    }

    writePC(newPC) {
        let intVal = 0;
        if (typeof newPC === "string") {
            if (newPC.startsWith("0x")) {
                intVal = parseInt(newPC, 16) | 0;
            } else if (/^[01]+$/.test(newPC)) {
                intVal = parseInt(newPC, 2) | 0;
            } else {
                intVal = parseInt(newPC, 10) | 0;
            }
        } else {
            intVal = Number(newPC) | 0;
        }
        this.pc = intVal;
    }

    // Retorna todos os 32 registradores como strings binárias de 32 bits
    readAll() {
        const arr = new Array(32);
        for (let i = 0; i < 32; i++) {
            arr[i] = this.read(i);
        }
        return arr;
    }

    // Retorna lista com metadados para facilitar exibição na UI React
    getAllWithMeta() {
        const list = [];
        for (let i = 0; i < 32; i++) {
            const valInt = this.readInt(i);
            const valUInt = this.readUInt(i);
            list.push({
                index: i,
                id: `x${i}`,
                name: `x${i}`,
                abi: ABI_NAMES[i],
                label: `x${i} (${ABI_NAMES[i]})`,
                bin: (valUInt).toString(2).padStart(32, "0"),
                hex: toHex32(valUInt),
                dec: valInt
            });
        }
        return list;
    }

    // Reseta todos os registradores e o PC para zero
    clear() {
        this.data.fill(0);
        this.pc = 0;
    }
}

export default Registers;
