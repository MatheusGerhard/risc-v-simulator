import { useState } from "react";
import MemoryDisplay from "./memoryDisplay";
import BinaryUploader from "./BinaryUploader";
import styles from "./styles.module.css";
import memoria from "../../../mac1/componentes/memory";
import clock from "../../../testes/teste3";
import { uc1, uc2, uc3 } from "../../../testes/teste3";
import { executa } from "../../../testes/teste3";
import { useMac } from "../../context/MacContext";
import useIPS from "../../hooks/useIPS";

export default function Memory() {
    const { activeMac } = useMac();
    const ucs = {
        mac1: uc1,
        mac2: uc2,
        mac3: uc3,
    };
    const ucAtual = ucs[activeMac];

    const [memoryNum, setMemoryNum] = useState([1, 0, 0]);
    const [isRunning, setIsRunning] = useState(false);

    const { formattedIPS, ips } = useIPS(ucAtual, isRunning);

    function addMemory(pos) {
        setMemoryNum((current) => {
            const newMemory = [...current];
            newMemory[pos] = 1;
            return newMemory;
        });
    }

    function deleteMemory(pos) {
        setMemoryNum((current) => {
            const newMemory = [...current];
            newMemory[pos] = 0;
            return newMemory;
        });
    }

    function resetComponents() {
        clock.resetClock();
        ucAtual.reset();
    }

    function clearMemory() {
        memoria.clearMemory();
        resetComponents();
        setIsRunning(false);
    }

    function initProgram() {
        executa(clock, ucAtual);
        setIsRunning(true);
    }

    function pauseProgram() {
        clock.pausar();
        setIsRunning(false);
    }

    // Manipulador do carregamento de arquivos .bin externos
    function handleLoadBinary(byteArray, baseAddrStr) {
        let baseAddr = 0;
        if (typeof baseAddrStr === "string") {
            if (baseAddrStr.startsWith("0x") || baseAddrStr.startsWith("0X")) {
                baseAddr = parseInt(baseAddrStr, 16);
            } else {
                baseAddr = parseInt(baseAddrStr, 10);
            }
        } else {
            baseAddr = Number(baseAddrStr) || 0;
        }

        // 1. Limpa a memória e injeta os bytes sequenciais em Little-Endian
        memoria.clearMemory();
        const bytesLoaded = memoria.loadRawBinary(byteArray, baseAddr);

        // 2. Aciona o Hard Reset: zera registradores, limpa flags e aponta PC para baseAddr
        clock.resetClock();
        ucAtual.hardReset(baseAddr);
        setIsRunning(false);

        return bytesLoaded;
    }

    return (
        <div className={styles.memory}>
            <div className={styles.content}>
                <div className={styles.header}>
                    <h1 className={styles.title}>Memória</h1>
                </div>
                <div className={styles.division}></div>

                {/* Painel de Controle Principal */}
                <div className={styles.container}>
                    <button className={styles.button} onClick={clearMemory} title="Limpar Memória e Registradores">
                        <h1 className={styles.buttonText}>Clear</h1>
                    </button>
                    <button className={`${styles.button} ${isRunning ? styles.buttonActive : ""}`} onClick={initProgram} title="Executar Simulação">
                        <h1 className={styles.buttonText}>►</h1>
                    </button>
                    <button className={styles.button} onClick={pauseProgram} title="Pausar Simulação">
                        <h1 className={styles.buttonText}>||</h1>
                    </button>
                    <input placeholder="Tempo (ms)" type="number" min="0" className={styles.timeInput} id="time"></input>
                    <button className={styles.button} onClick={() => clock.setVelocidade(document.getElementById("time").value)} title="Definir intervalo do Clock">
                        <h1 className={styles.buttonText}>Set Clock</h1>
                    </button>

                    <div className={styles.ipsDisplay} title={`${ips.toFixed(1)} instruções executadas por segundo (tempo real)`}>
                        <span className={styles.ipsLabel}>IPS Real</span>
                        <span className={styles.ipsValue}>{formattedIPS}</span>
                    </div>
                </div>

                <div className={styles.division}></div>

                {/* Componente de Importação de Binários .bin compilados externamente */}
                <div style={{ padding: "0 0.75rem" }}>
                    <BinaryUploader onLoadBinary={handleLoadBinary} />
                </div>

                <div className={styles.division}></div>

                {/* Exibições da RAM */}
                <div className={styles.container}>
                    <div className={styles.slot}>
                        {memoryNum[0] ? (
                            <MemoryDisplay
                                close={deleteMemory}
                                id={0}
                            />
                        ) : (
                            <button className={styles.addButton} onClick={() => addMemory(0)}>
                                <h1 className={styles.buttonText}>+</h1>
                            </button>
                        )}
                    </div>
                    <div className={styles.slot}>
                        {memoryNum[1] ? (
                            <MemoryDisplay
                                close={deleteMemory}
                                id={1}
                            />
                        ) : (
                            <button className={styles.addButton} onClick={() => addMemory(1)}>
                                <h1 className={styles.buttonText}>+</h1>
                            </button>
                        )}
                    </div>
                    <div className={styles.slot}>
                        {memoryNum[2] ? (
                            <MemoryDisplay
                                close={deleteMemory}
                                id={2}
                            />
                        ) : (
                            <button className={styles.addButton} onClick={() => addMemory(2)}>
                                <h1 className={styles.buttonText}>+</h1>
                            </button>
                        )}
                    </div>
                </div>
            </div>
        </div>
    );
}
