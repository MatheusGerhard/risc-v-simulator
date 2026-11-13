import memoria from '../mac1/componentes/memory.js';
import ControlUnit from '../mac1/controlUnit.js';
import Clock from '../mac1/componentes/clock.js';

console.log("=== TESTE DO CARREGADOR DE BINÁRIOS REAIS (.BIN FLAT BARE-METAL) ===");

// 1. Sintetiza um binário bruto equivalente ao gerado por:
// riscv64-unknown-elf-gcc -nostdlib -o prog.elf prog.s && riscv64-unknown-elf-objcopy -O binary prog.elf prog.bin
//
// Instruções:
// 0x0000: addi x1, x0, 100  (0x06400093 -> LE: 93 00 40 06)
// 0x0004: addi x2, x0, 250  (0x0FA00113 -> LE: 13 01 A0 0F)
// 0x0008: add  x3, x1, x2   (0x002081B3 -> LE: B3 81 20 00)
// 0x000C: sw   x3, 32(x0)   (0x02302023 -> LE: 23 20 30 02) [Salva na RAM no endereço 32]
// 0x0010: lw   x4, 32(x0)   (0x02002203 -> LE: 03 22 00 02) [Lê da RAM do endereço 32]

const rawBinaryBytes = new Uint8Array([
    0x93, 0x00, 0x40, 0x06, // addi x1, x0, 100
    0x13, 0x01, 0xA0, 0x0F, // addi x2, x0, 250
    0xB3, 0x81, 0x20, 0x00, // add  x3, x1, x2
    0x23, 0x20, 0x30, 0x02, // sw   x3, 32(x0)
    0x03, 0x22, 0x00, 0x02  // lw   x4, 32(x0)
]);

console.log(`\n[1] Carregando ${rawBinaryBytes.length} bytes na memória RAM a partir de 0x00000000...`);
memoria.clearMemory();
const bytesInjetados = memoria.loadRawBinary(rawBinaryBytes, 0x00000000);
if (bytesInjetados !== 20) throw new Error(`Esperado 20 bytes injetados, obtido ${bytesInjetados}`);

// Verifica injeção byte a byte Little-Endian (leitura sem sinal)
if (memoria.readByte(0, false) !== 0x93 || memoria.readByte(1, false) !== 0x00 || memoria.readByte(2, false) !== 0x40 || memoria.readByte(3, false) !== 0x06) {
    throw new Error("Primeira palavra na memória não corresponde aos bytes Little-Endian!");
}
if (memoria.readWord(0) !== 0x06400093) {
    throw new Error(`Palavra lida (0x${memoria.readWord(0).toString(16)}) não confere com 0x06400093`);
}
console.log("-> Bytes gravados e lidos corretamente em Little-Endian!");

// 2. Teste do Hard Reset do Processador
console.log("\n[2] Executando Hard Reset do Processador...");
const uc = new ControlUnit();
const clock = new Clock();

// Inicializa registradores com valores residuais para garantir que o hard reset limpa
uc.regs.write(1, 999);
uc.regs.write(2, 888);
uc.regs.write(3, 777);
uc.hardReset(0x00000000);

if (uc.regs.readInt(1) !== 0 || uc.regs.readInt(2) !== 0 || uc.regs.readInt(3) !== 0) {
    throw new Error("Hard Reset falhou em zerar os registradores!");
}
if (uc.regs.readPC() !== 0) {
    throw new Error(`Hard Reset deve apontar PC para 0x00000000, atual: ${uc.regs.readPC()}`);
}
if (uc.stageName !== "FETCH") {
    throw new Error("Hard Reset deve preparar o estágio FETCH");
}
console.log("-> Hard Reset validado com sucesso! Registradores limpos e PC apontado para 0x00000000.");

// 3. Execução das instruções do binário
console.log("\n[3] Executando instruções do binário carregado...");

// Instrução 1: addi x1, x0, 100
clock.pulso(uc);
if (uc.regs.readInt(1) !== 100) throw new Error(`x1 deve ser 100, obtido ${uc.regs.readInt(1)}`);
console.log("-> Instrução 1 executada: x1 = 100");

// Instrução 2: addi x2, x0, 250
clock.pulso(uc);
if (uc.regs.readInt(2) !== 250) throw new Error(`x2 deve ser 250, obtido ${uc.regs.readInt(2)}`);
console.log("-> Instrução 2 executada: x2 = 250");

// Instrução 3: add x3, x1, x2
clock.pulso(uc);
if (uc.regs.readInt(3) !== 350) throw new Error(`x3 deve ser 350, obtido ${uc.regs.readInt(3)}`);
console.log("-> Instrução 3 executada: x3 = 350");

// Instrução 4: sw x3, 32(x0)
clock.pulso(uc);
if (memoria.readWord(32) !== 350) throw new Error(`Memória[32] deve ser 350, obtido ${memoria.readWord(32)}`);
console.log("-> Instrução 4 executada: Memória[32] = 350");

// Instrução 5: lw x4, 32(x0)
clock.pulso(uc);
if (uc.regs.readInt(4) !== 350) throw new Error(`x4 deve ser 350, obtido ${uc.regs.readInt(4)}`);
console.log("-> Instrução 5 executada: x4 = 350");

// 4. Teste com Endereço Base Arbitrário (ex: 0x00000100)
console.log("\n[4] Testando carregamento com endereço base arbitrário (0x100)...");
const baseOffset = 0x100;
memoria.clearMemory();
memoria.loadRawBinary(rawBinaryBytes, baseOffset);
uc.hardReset(baseOffset);

if (uc.regs.readPC() !== baseOffset) {
    throw new Error(`PC deve iniciar em 0x${baseOffset.toString(16)}, atual: 0x${uc.regs.readPC().toString(16)}`);
}

clock.pulso(uc); // executa addi x1, x0, 100 no endereço 0x100
if (uc.regs.readInt(1) !== 100) throw new Error("Execução em endereço base arbitrário falhou");
if (uc.regs.readPC() !== baseOffset + 4) throw new Error("PC deve ter avançado para 0x104");

console.log(`-> Sucesso! Binário executado a partir do endereço base 0x${baseOffset.toString(16)}.`);
console.log("\n=== TODOS OS TESTES DO CARREGADOR DE BINÁRIOS PASSARAM COM 100% DE SUCESSO! ===");
