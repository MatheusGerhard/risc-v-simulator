import ControlUnit from '../mac1/controlUnit.js';
import Clock from '../mac1/componentes/clock.js';
import Montador from '../montador/montador.js';
import memoria from '../mac1/componentes/memory.js';
import { formatIPS } from '../src/hooks/useIPS.js';

console.log("=== TESTE DO CONTADOR DE INSTRUÇÕES E FORMATADOR IPS ===");

// 1. Teste do Formatador de IPS
console.log("\n[1] Testando formatIPS...");
if (formatIPS(0, false) !== "0 IPS") throw new Error("0 quando pausado deve ser 0 IPS");
if (formatIPS(0, true) !== "0 IPS") throw new Error("0 deve ser 0 IPS");
if (formatIPS(5.2, true) !== "5.2 IPS") throw new Error("5.2 deve ser 5.2 IPS");
if (formatIPS(25, true) !== "25 IPS") throw new Error("25 deve ser 25 IPS");
if (formatIPS(1200, true) !== "1.2k IPS") throw new Error("1200 deve ser 1.2k IPS");
if (formatIPS(50000, true) !== "50.0k IPS") throw new Error("50000 deve ser 50.0k IPS");
if (formatIPS(2500000, true) !== "2.5M IPS") throw new Error("2500000 deve ser 2.5M IPS");
console.log("-> formatIPS passou em todos os casos de teste!");

// 2. Teste do Contador de Instruções no Ciclo de FSM
console.log("\n[2] Testando incremento no estágio de Write-back...");
const uc = new ControlUnit();
const clock = new Clock();

const assembly = `
    addi x1, x0, 10
    addi x2, x0, 20
    add  x3, x1, x2
`;

const montador = new Montador(assembly);
montador.main();
memoria.clearMemory();
montador.preencherMemoria();

if (uc.getCompletedInstructions() !== 0) throw new Error("Contador deve iniciar em 0");

// Subciclos 1, 2, 3, 4 (ainda não terminou a primeira instrução)
for (let sc = 1; sc <= 4; sc++) {
    clock.subPulso(uc);
    if (uc.getCompletedInstructions() !== 0) {
        throw new Error(`Contador não deve incrementar no subciclo ${sc}`);
    }
}

// Subciclo 5 (Write-back) - deve incrementar!
clock.subPulso(uc);
if (uc.getCompletedInstructions() !== 1) {
    throw new Error(`Esperado 1 instrução concluída, obtido: ${uc.getCompletedInstructions()}`);
}
console.log("-> Instrução 1 concluída com sucesso no subciclo 5 (Write-back)!");

// Executa instrução 2 completa via pulso
clock.pulso(uc);
if (uc.getCompletedInstructions() !== 2) {
    throw new Error(`Esperado 2 instruções concluídas, obtido: ${uc.getCompletedInstructions()}`);
}
console.log("-> Instrução 2 concluída com sucesso!");

// Executa instrução 3 completa via pulso
clock.pulso(uc);
if (uc.getCompletedInstructions() !== 3) {
    throw new Error(`Esperado 3 instruções concluídas, obtido: ${uc.getCompletedInstructions()}`);
}
console.log("-> Instrução 3 concluída com sucesso!");

// Teste de Reset
uc.reset();
if (uc.getCompletedInstructions() !== 0) {
    throw new Error("Reset deve zerar o contador de instruções");
}
console.log("-> Reset zerou o contador com sucesso!");

console.log("\n=== TODOS OS TESTES DE IPS PASSARAM COM SUCESSO! ===");
