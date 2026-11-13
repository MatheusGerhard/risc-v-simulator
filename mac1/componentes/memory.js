// Main Memory - Memória Principal (RAM) do RISC-V RV32I
// Descrição: Memória endereçável por byte (byte-addressable) com arquitetura Little-Endian.
// Suporta leitura e escrita de:
// - Bytes (8 bits) - LB (com extensão de sinal), LBU (sem sinal), SB
// - Half-words (16 bits) - LH (com extensão de sinal), LHU (sem sinal), SH
// - Words (32 bits) - LW, SW

import { clearMacroInstructionCatalog, logMemoryWrite } from "../../src/services/simulationLog.js";

export class Memory {
    constructor(sizeInBytes = 128) {
        this.size = sizeInBytes; // padrão: 64 KB
        this.bytes = new Uint8Array(this.size);
        this.data = []; // Cache/array de palavras de 32-bits para integração com a UI React
        this.refreshWordData();
    }

    // Normaliza endereço (suporta string binária, hex ou número inteiro)
    parseAddress(address) {
        if (typeof address === "number") return address;
        if (typeof address === "string") {
            if (address.startsWith("0x") || address.startsWith("0X")) {
                return parseInt(address, 16);
            }
            if (/^[01]+$/.test(address)) {
                return parseInt(address, 2);
            }
            return parseInt(address, 10);
        }
        return 0;
    }

    // Atualiza o espelho de palavras de 32 bits (para a UI)
    refreshWordData() {
        const numWords = Math.min(4096, Math.floor(this.size / 4));
        if (this.data.length !== numWords) {
            this.data = new Array(numWords);
        }
        for (let i = 0; i < numWords; i++) {
            this.data[i] = this.readWordBinary(i * 4);
        }
    }

    // ─────────────────────────────────────────────
    // Leituras (Little-Endian)
    // ─────────────────────────────────────────────

    // Leitura de 1 byte (LB / LBU)
    readByte(address, signed = true) {
        const addr = this.parseAddress(address) & (this.size - 1);
        const b = this.bytes[addr];
        return signed ? ((b << 24) >> 24) : (b >>> 0);
    }

    // Leitura de 2 bytes (LH / LHU)
    readHalf(address, signed = true) {
        const addr = this.parseAddress(address) & (this.size - 1);
        const b0 = this.bytes[addr];
        const b1 = this.bytes[(addr + 1) & (this.size - 1)];
        const val = b0 | (b1 << 8);
        return signed ? ((val << 16) >> 16) : (val >>> 0);
    }

    // Leitura de 4 bytes (LW) - retorna inteiro de 32 bits com sinal
    readWord(address) {
        const addr = this.parseAddress(address) & (this.size - 1);
        const b0 = this.bytes[addr];
        const b1 = this.bytes[(addr + 1) & (this.size - 1)];
        const b2 = this.bytes[(addr + 2) & (this.size - 1)];
        const b3 = this.bytes[(addr + 3) & (this.size - 1)];
        return (b0 | (b1 << 8) | (b2 << 16) | (b3 << 24)) | 0;
    }

    // Leitura de 4 bytes sem sinal
    readWordUnsigned(address) {
        return this.readWord(address) >>> 0;
    }

    // Leitura de palavra em string binária de 32 bits
    readWordBinary(address) {
        return (this.readWordUnsigned(address)).toString(2).padStart(32, "0");
    }

    // Leitura de palavra em Hexadecimal
    readWordHex(address) {
        return "0x" + (this.readWordUnsigned(address)).toString(16).padStart(8, "0").toUpperCase();
    }

    // Compatibilidade com interface padrão: read(address) retorna string binária de 32 bits
    read(address) {
        return this.readWordBinary(address);
    }

    // ─────────────────────────────────────────────
    // Escritas (Little-Endian)
    // ─────────────────────────────────────────────

    // Escrita de 1 byte (SB)
    writeByte(address, value) {
        const addr = this.parseAddress(address) & (this.size - 1);
        const val = (typeof value === "string" ? parseInt(value, 2) : Number(value)) & 0xFF;
        const prevWord = this.readWordBinary(addr & ~3);

        this.bytes[addr] = val;

        const wordIdx = Math.floor(addr / 4);
        if (wordIdx < this.data.length) {
            this.data[wordIdx] = this.readWordBinary(wordIdx * 4);
        }

        const newWord = this.readWordBinary(addr & ~3);
        logMemoryWrite(addr, prevWord, newWord);
    }

    // Escrita de 2 bytes (SH)
    writeHalf(address, value) {
        const addr = this.parseAddress(address) & (this.size - 1);
        const val = typeof value === "string" ? parseInt(value, 2) : Number(value);
        const prevWord = this.readWordBinary(addr & ~3);

        this.bytes[addr] = val & 0xFF;
        this.bytes[(addr + 1) & (this.size - 1)] = (val >> 8) & 0xFF;

        const wordIdx = Math.floor(addr / 4);
        if (wordIdx < this.data.length) {
            this.data[wordIdx] = this.readWordBinary(wordIdx * 4);
        }

        const newWord = this.readWordBinary(addr & ~3);
        logMemoryWrite(addr, prevWord, newWord);
    }

    // Escrita de 4 bytes (SW)
    writeWord(address, value) {
        const addr = this.parseAddress(address) & (this.size - 1);
        let intVal = 0;
        if (typeof value === "string") {
            if (value.startsWith("0x")) {
                intVal = parseInt(value, 16) | 0;
            } else if (/^[01]+$/.test(value)) {
                intVal = parseInt(value, 2) | 0;
            } else {
                intVal = parseInt(value, 10) | 0;
            }
        } else {
            intVal = Number(value) | 0;
        }

        const prevWord = this.readWordBinary(addr);

        this.bytes[addr] = intVal & 0xFF;
        this.bytes[(addr + 1) & (this.size - 1)] = (intVal >> 8) & 0xFF;
        this.bytes[(addr + 2) & (this.size - 1)] = (intVal >> 16) & 0xFF;
        this.bytes[(addr + 3) & (this.size - 1)] = (intVal >> 24) & 0xFF;

        const wordIdx = Math.floor(addr / 4);
        if (wordIdx < this.data.length) {
            this.data[wordIdx] = this.readWordBinary(wordIdx * 4);
        }

        const newWord = this.readWordBinary(addr);
        logMemoryWrite(addr, prevWord, newWord);
    }

    // Método padrão de escrita compatível
    write(address, value) {
        this.writeWord(address, value);
    }

    // Escrita utilizada pelo Montador (garante endereço byte-aligned de 4 bytes)
    writeMontador(address, value) {
        let addr = this.parseAddress(address);
        // Se o endereço recebido for o índice de instrução (ex: 0, 1, 2) e não endereço de byte múltiplo de 4,
        // ajustamos se for necessário, mas no RV32I o montador usa endereços em bytes.
        this.writeWord(addr, value);
    }

    clearMemory() {
        this.bytes.fill(0);
        this.refreshWordData();
        clearMacroInstructionCatalog();
    }

    // Injeta um binário bruto (.bin / flat binary bare-metal) na memória
    loadRawBinary(byteArray, baseAddress = 0) {
        const bytes = byteArray instanceof Uint8Array ? byteArray : new Uint8Array(byteArray);
        const startAddr = this.parseAddress(baseAddress) & (this.size - 1);

        for (let i = 0; i < bytes.length; i++) {
            const targetAddr = (startAddr + i) & (this.size - 1);
            this.bytes[targetAddr] = bytes[i];
        }

        this.refreshWordData();
        clearMacroInstructionCatalog();
        return bytes.length;
    }

    preencheInstrucoes(microinstrucoes, macroinstrucoes) {
        for (let i = 0; i < macroinstrucoes.length; i++) {
            if (macroinstrucoes[i].rotulo != null && macroinstrucoes[i].opcode === null) continue;
            // Se o endereço fornecido for relativo a instrução (0, 1, 2...), converte para endereço em bytes (* 4)
            let byteAddress = macroinstrucoes[i].byteAddress ?? (macroinstrucoes[i].rotulo_index * 4);
            if (macroinstrucoes[i].byteAddress === undefined && macroinstrucoes[i].rotulo_index !== undefined) {
                // Se já for múltiplo de 4, verifica
                byteAddress = macroinstrucoes[i].rotulo_index;
            }
            this.writeMontador(byteAddress, microinstrucoes[i]);
        }
        this.refreshWordData();
    }
}

const memoria = new Memory();

export default memoria;
