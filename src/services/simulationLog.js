import {
    resetPerformanceTracker,
    trackInstructionStart,
} from "./performanceTracker.js";
import { ABI_NAMES } from "../../mac1/componentes/registers.js";

const MAX_LOG_EVENTS = 500;

const LOG_LABELS = {
    memoryWrite: "[MEMORY WRITE]",
    cacheHit: "[CACHE HIT]",
    cacheMiss: "[CACHE MISS]",
    instruction: "[INSTRUCTION]",
    instructionStarted: "[INSTRUCTION STARTED]",
    instructionFinished: "[INSTRUCTION FINISHED]",
};

let events = [];
let nextId = 1;
let subscribers = new Set();
let instructionCatalog = new Map();
let lastInstructionLogKey = null;

function normalizeBinaryWord(value) {
    return String(value ?? "").padStart(32, "0").slice(-32);
}

function normalizeAddress(address) {
    if (typeof address === "number") {
        return address;
    }

    const value = String(address ?? "");

    if (/^[01]+$/.test(value)) {
        return parseInt(value, 2);
    }

    const decimal = Number(value);
    return Number.isNaN(decimal) ? value : decimal;
}

function formatInstruction(instruction) {
    if (!instruction || !instruction.opcode) {
        return null;
    }

    if (instruction.operands && Array.isArray(instruction.operands)) {
        return `${instruction.opcode} ${instruction.operands.join(", ")}`;
    }

    if (instruction.operando === null || instruction.operando === undefined) {
        return instruction.opcode;
    }

    return `${instruction.opcode} ${instruction.operando}`;
}

function decodeInstructionWord(word) {
    const binaryWord = normalizeBinaryWord(word);
    const intVal = parseInt(binaryWord, 2) | 0;
    if (intVal === 0) return { opcode: "NOP", operando: null };

    const opcode = intVal & 0x7F;
    const rd = (intVal >>> 7) & 0x1F;
    const funct3 = (intVal >>> 12) & 0x07;
    const rs1 = (intVal >>> 15) & 0x1F;
    const rs2 = (intVal >>> 20) & 0x1F;
    const funct7 = (intVal >>> 25) & 0x7F;
    const immI = (intVal >> 20);

    const rdName = `x${rd} (${ABI_NAMES[rd] || ""})`;
    const rs1Name = `x${rs1} (${ABI_NAMES[rs1] || ""})`;
    const rs2Name = `x${rs2} (${ABI_NAMES[rs2] || ""})`;

    if (opcode === 0x33) {
        let op = "ADD";
        if (funct3 === 0x0) op = funct7 === 0x20 ? "SUB" : "ADD";
        else if (funct3 === 0x1) op = "SLL";
        else if (funct3 === 0x2) op = "SLT";
        else if (funct3 === 0x3) op = "SLTU";
        else if (funct3 === 0x4) op = "XOR";
        else if (funct3 === 0x5) op = funct7 === 0x20 ? "SRA" : "SRL";
        else if (funct3 === 0x6) op = "OR";
        else if (funct3 === 0x7) op = "AND";
        return { opcode: op, operando: `${rdName}, ${rs1Name}, ${rs2Name}` };
    }

    if (opcode === 0x13) {
        let op = "ADDI";
        if (funct3 === 0x0) op = "ADDI";
        else if (funct3 === 0x1) op = "SLLI";
        else if (funct3 === 0x2) op = "SLTI";
        else if (funct3 === 0x3) op = "SLTIU";
        else if (funct3 === 0x4) op = "XORI";
        else if (funct3 === 0x5) op = funct7 === 0x20 ? "SRAI" : "SRLI";
        else if (funct3 === 0x6) op = "ORI";
        else if (funct3 === 0x7) op = "ANDI";
        return { opcode: op, operando: `${rdName}, ${rs1Name}, ${immI}` };
    }

    if (opcode === 0x03) {
        let op = "LW";
        if (funct3 === 0x0) op = "LB";
        else if (funct3 === 0x1) op = "LH";
        else if (funct3 === 0x2) op = "LW";
        else if (funct3 === 0x4) op = "LBU";
        else if (funct3 === 0x5) op = "LHU";
        return { opcode: op, operando: `${rdName}, ${immI}(${rs1Name})` };
    }

    if (opcode === 0x23) {
        const immS = (((intVal >> 25) << 5) | ((intVal >>> 7) & 0x1F) << 20) >> 20;
        let op = "SW";
        if (funct3 === 0x0) op = "SB";
        else if (funct3 === 0x1) op = "SH";
        else if (funct3 === 0x2) op = "SW";
        return { opcode: op, operando: `${rs2Name}, ${immS}(${rs1Name})` };
    }

    if (opcode === 0x63) {
        let op = "BEQ";
        if (funct3 === 0x0) op = "BEQ";
        else if (funct3 === 0x1) op = "BNE";
        else if (funct3 === 0x4) op = "BLT";
        else if (funct3 === 0x5) op = "BGE";
        else if (funct3 === 0x6) op = "BLTU";
        else if (funct3 === 0x7) op = "BGEU";
        return { opcode: op, operando: `${rs1Name}, ${rs2Name}` };
    }

    if (opcode === 0x37) return { opcode: "LUI", operando: `${rdName}, 0x${((intVal >>> 12) & 0xFFFFF).toString(16)}` };
    if (opcode === 0x17) return { opcode: "AUIPC", operando: `${rdName}, 0x${((intVal >>> 12) & 0xFFFFF).toString(16)}` };
    if (opcode === 0x6F) return { opcode: "JAL", operando: `${rdName}` };
    if (opcode === 0x67) return { opcode: "JALR", operando: `${rdName}, ${rs1Name}, ${immI}` };

    return null;
}

function notifySubscribers() {
    const snapshot = [...events];
    subscribers.forEach((callback) => callback(snapshot));
}

function addLogEvent(type, message, metadata = {}) {
    const event = {
        id: nextId,
        label: LOG_LABELS[type] ?? "[LOG]",
        message,
        type,
        metadata,
        timestamp: new Date().toLocaleTimeString("pt-BR", {
            hour12: false,
        }),
    };

    nextId += 1;
    events = [event, ...events].slice(0, MAX_LOG_EVENTS);
    notifySubscribers();
}

export function subscribeToSimulationLog(callback) {
    subscribers.add(callback);
    callback([...events]);

    return () => {
        subscribers.delete(callback);
    };
}

export function clearSimulationLog() {
    events = [];
    nextId = 1;
    lastInstructionLogKey = null;
    resetPerformanceTracker();
    notifySubscribers();
}

export function clearMacroInstructionCatalog() {
    instructionCatalog.clear();
    lastInstructionLogKey = null;
    resetPerformanceTracker();
}

export function registerMacroInstructions(instructions = []) {
    instructionCatalog.clear();

    instructions.forEach((instruction, index) => {
        const address =
            instruction.byteAddress ??
            instruction.rotulo_index ??
            instruction.address ??
            (index * 4);

        if (address !== null && address !== undefined) {
            instructionCatalog.set(normalizeAddress(address), instruction);
        }
    });
}

export function logInstructionExecution(address, details = {}) {
    const normalizedAddress = normalizeAddress(address);
    const executionKey = details.executionKey ?? `${details.processor ?? "CPU"}:${normalizedAddress}`;

    if (lastInstructionLogKey === executionKey) {
        return;
    }

    lastInstructionLogKey = executionKey;

    const catalogInstruction = instructionCatalog.get(normalizedAddress);
    const decodedInstruction = catalogInstruction
        ? null
        : decodeInstructionWord(details.word);
    const instruction = catalogInstruction ?? decodedInstruction;
    const formattedInstruction = formatInstruction(instruction);

    const description = formattedInstruction
        ? `Executando: ${formattedInstruction}`
        : `Executando instrução no endereço 0x${normalizedAddress.toString(16).toUpperCase()}`;

    trackInstructionStart(details.processor ?? "RV32I", {
        address: normalizedAddress,
        cycle: details.cycle,
        instruction: formattedInstruction,
        cacheHits: details.cacheHits,
        cacheMisses: details.cacheMisses,
    });

    addLogEvent("instruction", description, {
        address: normalizedAddress,
        cycle: details.cycle,
        processor: details.processor,
        instruction: formattedInstruction,
        opcode: instruction?.opcode,
        operando: instruction?.operando ?? instruction?.imm,
    });
}

export function logMemoryWrite(address, previousValue, value) {
    const normalizedAddress = normalizeAddress(address);
    addLogEvent(
        "memoryWrite",
        `RAM[0x${normalizedAddress.toString(16).toUpperCase()}]: ${previousValue} -> ${value}`,
        {
            address: normalizedAddress,
            previousValue,
            value,
        },
    );
}

export function logCacheHit(processor, address) {
    addLogEvent("cacheHit", `${processor}: Hit no endereço 0x${normalizeAddress(address).toString(16).toUpperCase()}`);
}

export function logCacheMiss(processor, address) {
    addLogEvent("cacheMiss", `${processor}: Miss no endereço 0x${normalizeAddress(address).toString(16).toUpperCase()}`);
}

export function getSimulationLog() {
    return [...events];
}
