import Registers from '../mac1/componentes/registers.js';
import memoria from '../mac1/componentes/memory.js';
import ArithmeticLogicUnit from '../mac1/componentes/alu.js';
import Montador from '../montador/montador.js';
import ControlUnit from '../mac1/controlUnit.js';
import Clock from '../mac1/componentes/clock.js';

console.log("=== BATERIA COMPLETA DE TESTES DA ISA RV32I ===");

// Programa de teste completo
const codigo = `
# 1. Teste LUI e AUIPC
lui  x1, 0x12345
auipc x2, 0x1000

# 2. Teste Aritmética e Imediatos
addi x3, x0, 10
addi x4, x3, -4
sub  x5, x3, x4
xor  x6, x3, x4
or   x7, x3, x4
and  x8, x3, x4

# 3. Teste Shifts
slli x9, x3, 2
srli x10, x9, 1
srai x11, x4, 1

# 4. Teste Comparações
slti x12, x4, 10
sltu x13, x4, x3

# 5. Teste Memória (SB, SH, SW, LB, LH, LW, LBU, LHU)
addi x14, x0, 0x100
sw   x3, 0(x14)
lw   x15, 0(x14)
sh   x3, 4(x14)
lhu  x16, 4(x14)
sb   x3, 6(x14)
lbu  x17, 6(x14)

# 6. Teste Desvios Condicionais (B-Type)
beq  x3, x15, branch_ok
addi x18, x0, 1 # Deve ser pulado
branch_ok:
addi x18, x0, 2

# 7. Teste JAL e JALR
jal  x1, subrotina
addi x19, x0, 100
j fim

subrotina:
addi x20, x0, 77
ret

fim:
addi x21, x0, 999
`;

const montador = new Montador(codigo);
const erro = montador.main();
if (erro !== false) {
    console.error("Erro no montador:", erro);
    process.exit(1);
}
console.log(`Montagem OK: ${montador.microinstrucoes.length} instruções geradas.`);

memoria.clearMemory();
montador.preencherMemoria();

const uc = new ControlUnit();
const clock = new Clock();

// Executa todas as instruções até atingir o fim
let limiteInstrucoes = 50;
let instrCount = 0;

while (instrCount < limiteInstrucoes) {
    clock.pulso(uc);
    instrCount++;
    if (uc.regs.readInt(21) === 999) {
        console.log(`Fim do programa alcançado com sucesso após ${instrCount} instruções!`);
        break;
    }
}

if (uc.regs.readInt(21) !== 999) {
    console.error(`Falha: instrução final não atingida. PC=${uc.regs.readPC()}`);
    process.exit(1);
}

// Verificação dos valores finais
console.log("\nVerificação dos Registradores:");
console.log("x1 (lui):", uc.regs.readHex(1));
console.log("x3 (10):", uc.regs.readInt(3));
console.log("x4 (10 - 4 = 6):", uc.regs.readInt(4));
console.log("x5 (10 - 6 = 4):", uc.regs.readInt(5));
console.log("x9 (10 << 2 = 40):", uc.regs.readInt(9));
console.log("x10 (40 >> 1 = 20):", uc.regs.readInt(10));
console.log("x12 (6 < 10 = 1):", uc.regs.readInt(12));
console.log("x15 (lw da ram = 10):", uc.regs.readInt(15));
console.log("x16 (lhu da ram = 10):", uc.regs.readInt(16));
console.log("x17 (lbu da ram = 10):", uc.regs.readInt(17));
console.log("x18 (branch_ok = 2):", uc.regs.readInt(18));
console.log("x20 (subrotina executada = 77):", uc.regs.readInt(20));
console.log("x21 (fim = 999):", uc.regs.readInt(21));

if (uc.regs.readInt(3) !== 10) throw new Error("x3 inválido");
if (uc.regs.readInt(4) !== 6) throw new Error("x4 inválido");
if (uc.regs.readInt(5) !== 4) throw new Error("x5 inválido");
if (uc.regs.readInt(9) !== 40) throw new Error("x9 inválido");
if (uc.regs.readInt(10) !== 20) throw new Error("x10 inválido");
if (uc.regs.readInt(12) !== 1) throw new Error("x12 inválido");
if (uc.regs.readInt(15) !== 10) throw new Error("x15 inválido");
if (uc.regs.readInt(16) !== 10) throw new Error("x16 inválido");
if (uc.regs.readInt(17) !== 10) throw new Error("x17 inválido");
if (uc.regs.readInt(18) !== 2) throw new Error("x18 inválido");
if (uc.regs.readInt(20) !== 77) throw new Error("x20 inválido");
if (uc.regs.readInt(21) !== 999) throw new Error("x21 inválido");

console.log("\n=== TODAS AS ASSERÇÕES PASSARAM COM 100% DE SUCESSO! ===");
