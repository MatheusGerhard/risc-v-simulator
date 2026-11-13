import styles from "./styles.module.css";
import Montador from "../../../../montador/montador";
import { useState } from "react";

const DEFAULT_PROGRAM = `# Programa Exemplo RV32I
# Demonstração: ADDI, ADD, SW, LW, BEQ

addi x1, x0, 10      # x1 = 10
addi x2, x0, 20      # x2 = 20
add  x3, x1, x2      # x3 = x1 + x2 = 30
sw   x3, 0(x0)       # Grava 30 no endereço de RAM 0
lw   x4, 0(x0)       # Lê palavra da RAM para x4
beq  x3, x4, sucesso # Se x3 == x4, desvia para 'sucesso'
addi x5, x0, 99      # (Pulado pelo desvio)
sucesso:
addi x5, x0, 42      # x5 = 42 (Sucesso!)
`;

export default function InstructionDisplay() {
    const [code, setCode] = useState(DEFAULT_PROGRAM);

    function handleClick() {
        let montador = new Montador(code);

        let result = montador.main();

        if (result != null) {
            if (result) {
                alert(result);
                return;
            }
        } else if (result === null) {
            alert("Erro na montagem do programa. Verifique o console para mais detalhes.");
            return;
        }

        montador.preencherMemoria();
        alert(`Programa RV32I montado com sucesso! (${montador.microinstrucoes.length} instruções carregadas na memória).`);
    }

    return (
        <div className={styles.instructions}>
            <button className={styles.sendButton} onClick={handleClick}>
                Montar Programa RV32I
            </button>
            <textarea
                value={code}
                onChange={(e) => setCode(e.target.value)}
                className={styles.textarea}
                placeholder="Insira código assembly RISC-V RV32I..."
            />
        </div>
    );
}
