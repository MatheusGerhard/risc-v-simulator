import Registers, { ABI_NAMES } from '../mac1/componentes/registers.js';
import memoria from '../mac1/componentes/memory.js';
import ArithmeticLogicUnit from '../mac1/componentes/alu.js';
import Montador from '../montador/montador.js';
import ControlUnit from '../mac1/controlUnit.js';
import Clock from '../mac1/componentes/clock.js';

console.log("=== INICIANDO TESTES DO MOTOR RV32I ===");

// 1. Teste de Registradores
console.log("\n[1] Teste de Registradores...");
const regs = new Registers();
regs.write(0, 100);
if (regs.readInt(0) !== 0) throw new Error("x0 deve ser sempre 0!");
regs.write(1, 42);
if (regs.readInt(1) !== 42) throw new Error("x1 (ra) deve ser 42");
regs.write("sp", 0x1000);
if (regs.readInt(2) !== 0x1000) throw new Error("x2 (sp) deve ser 0x1000");
regs.writePC(0x40);
if (regs.readPC() !== 0x40) throw new Error("PC deve ser 0x40");
console.log("-> Registradores OK!");

// 2. Teste de Memória Little-Endian
console.log("\n[2] Teste de Memória Little-Endian...");
memoria.clearMemory();
// Grava palavra 0x12345678 no endereço 0x100
// Little endian em bytes: 0x78, 0x56, 0x34, 0x12
memoria.writeWord(0x100, 0x12345678);
if (memoria.readByte(0x100, false) !== 0x78) throw new Error("Byte 0x100 deve ser 0x78");
if (memoria.readByte(0x101, false) !== 0x56) throw new Error("Byte 0x101 deve ser 0x56");
if (memoria.readByte(0x102, false) !== 0x34) throw new Error("Byte 0x102 deve ser 0x34");
if (memoria.readByte(0x103, false) !== 0x12) throw new Error("Byte 0x103 deve ser 0x12");
if (memoria.readHalf(0x100, false) !== 0x5678) throw new Error("Half-word deve ser 0x5678");
if (memoria.readWord(0x100) !== 0x12345678) throw new Error("Word deve ser 0x12345678");
console.log("-> Memória Little-Endian OK!");

// 3. Teste de ALU
console.log("\n[3] Teste de ALU...");
const alu = new ArithmeticLogicUnit();
alu.write(10, 20, "ADD");
if (alu.calcular() !== 30) throw new Error("ADD falhou");
alu.write(10, 20, "SUB");
if (alu.calcular() !== -10) throw new Error("SUB falhou");
alu.write(-5, 0, "SLT");
if (alu.calcular() !== 1) throw new Error("SLT falhou");
alu.write(-5, 0, "SLTU");
if (alu.calcular() !== 0) throw new Error("SLTU falhou (unsigned -5 é grande)");
alu.write(1, 4, "SLL");
if (alu.calcular() !== 16) throw new Error("SLL falhou");
alu.write(16, 2, "SRL");
if (alu.calcular() !== 4) throw new Error("SRL falhou");
alu.write(-16, 2, "SRA");
if (alu.calcular() !== -4) throw new Error("SRA falhou");
console.log("-> ALU OK!");

// 4. Teste do Montador
console.log("\n[4] Teste do Montador...");
const codigoAssembly = `
    addi x1, x0, 15
    addi x2, x0, 25
    add x3, x1, x2
    sw x3, 0(x0)
    lw x4, 0(x0)
    beq x3, x4, sucesso
    addi x5, x0, 99
sucesso:
    addi x5, x0, 100
`;

const montador = new Montador(codigoAssembly);
const resMontagem = montador.main();
if (resMontagem !== false) throw new Error("Montagem falhou: " + resMontagem);
console.log("Instruções montadas:", montador.microinstrucoes.length);
montador.preencherMemoria();
console.log("-> Montador OK!");

// 5. Teste da Unidade de Controle e Ciclo de Execução
console.log("\n[5] Teste de Execução Completa na ControlUnit...");
const uc = new ControlUnit();
const clock = new Clock();

// Executa instrução 1: addi x1, x0, 15
clock.pulso(uc);
if (uc.regs.readInt(1) !== 15) throw new Error(`Esperado x1=15, obtido ${uc.regs.readInt(1)}`);

// Executa instrução 2: addi x2, x0, 25
clock.pulso(uc);
if (uc.regs.readInt(2) !== 25) throw new Error(`Esperado x2=25, obtido ${uc.regs.readInt(2)}`);

// Executa instrução 3: add x3, x1, x2
clock.pulso(uc);
if (uc.regs.readInt(3) !== 40) throw new Error(`Esperado x3=40, obtido ${uc.regs.readInt(3)}`);

// Executa instrução 4: sw x3, 0(x0)
clock.pulso(uc);
if (memoria.readWord(0) !== 40) throw new Error(`Esperado memoria[0]=40, obtido ${memoria.readWord(0)}`);

// Executa instrução 5: lw x4, 0(x0)
clock.pulso(uc);
if (uc.regs.readInt(4) !== 40) throw new Error(`Esperado x4=40, obtido ${uc.regs.readInt(4)}`);

// Executa instrução 6: beq x3, x4, sucesso (deve pular a instrução addi x5, x0, 99)
clock.pulso(uc);
console.log("PC após BEQ:", uc.regs.readPC());

// Executa instrução 7 (sucesso): addi x5, x0, 100
clock.pulso(uc);
if (uc.regs.readInt(5) !== 100) throw new Error(`Esperado x5=100, obtido ${uc.regs.readInt(5)}`);

console.log("-> Execução Completa e Desvios Funcionando Perfeitamente!");
console.log("=== TODOS OS TESTES PASSARAM COM SUCESSO! ===");
