import { useEffect, useRef, useState } from "react";
import memoria from "../../../../mac1/componentes/memory";
import styles from './styles.module.css';

export default function MemoryDisplay({ id, close }) {
    const [data, setData] = useState(memoria.data);
    const [visibleLines, setVisibleLines] = useState(100);
    const [displayMode, setDisplayMode] = useState("hex"); // "hex", "bin", "dec"
    const [isSettingOpen, setIsSettingOpen] = useState(false);
    const [line, setLine] = useState(0);
    const lineRefs = useRef([]);

    useEffect(() => {
        const interval = setInterval(() => {
            setData([...memoria.data]); // cópia nova do array a cada 100ms
        }, 100);
        return () => clearInterval(interval);
    }, []);

    function changeBase() {
        if (displayMode === "hex") setDisplayMode("bin");
        else if (displayMode === "bin") setDisplayMode("dec");
        else setDisplayMode("hex");
    }

    function formatValue(value) {
        const binaryValue = String(value || "0").padStart(32, '0').slice(-32);

        if (displayMode === "bin") {
            return binaryValue;
        }

        const uintVal = parseInt(binaryValue, 2) >>> 0;

        if (displayMode === "hex") {
            return "0x" + uintVal.toString(16).padStart(8, '0').toUpperCase();
        }

        // decimal com sinal
        const intVal = parseInt(binaryValue, 2) | 0;
        return intVal;
    }

    function openSetting() {
        setIsSettingOpen(current => !current);
    }

    function searchLine() {
        let lineNumber = Number(line);

        if (Number.isNaN(lineNumber) || lineNumber < 0) {
            return;
        }

        // Se o usuário digitou um endereço de byte (múltiplo de 4 maior que o tamanho da lista)
        if (lineNumber >= data.length && lineNumber % 4 === 0) {
            lineNumber = Math.floor(lineNumber / 4);
        }

        if (lineNumber >= data.length) return;

        setVisibleLines(Math.max(visibleLines, lineNumber + 1));

        setTimeout(() => {
            lineRefs.current[lineNumber]?.scrollIntoView({
                behavior: "smooth",
                block: "center"
            });
        }, 0);
    }

    return (
        <div className={styles.memory}>
            <div className={styles.header}>
                <div className={styles.headerButtons}>
                    <button
                        className={styles.button}
                        onClick={changeBase}
                        title={`Formato atual: ${displayMode.toUpperCase()}. Clique para alternar.`}
                    >
                        <span style={{ fontSize: "11px", fontWeight: "bold", color: "#fff" }}>
                            {displayMode.toUpperCase()}
                        </span>
                    </button>
                    <button className={styles.button} onClick={openSetting} title="Buscar endereço">
                        <svg width="24" height="24" viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                            <path d="M12 13.2L16.6 8.6L18 10L12 16L6 10L7.4 8.6L12 13.2Z" fill="#ffffff"/>
                        </svg>
                    </button>
                </div>
                <div className={styles.title}>
                    RAM RV32I (Word 32-bit)
                </div>
                <button className={styles.button} onClick={() => close(id)}>
                    <h1 className={styles.buttonText}>×</h1>
                </button>
            </div>
            <div className={styles.container}>
                {
                    data.slice(0, visibleLines).map((value, index) => {
                        const byteAddr = index * 4;
                        const hexAddr = "0x" + byteAddr.toString(16).padStart(4, '0').toUpperCase();

                        return (
                            <div key={index} ref={(element) => lineRefs.current[index] = element}>
                                <div className={styles.adress}>
                                    <div className={styles.index} title={`Endereço de Byte: ${byteAddr}`}>
                                        {hexAddr}:
                                    </div>
                                    <div className={styles.value}>
                                        {formatValue(value)}
                                    </div>
                                </div>
                                <div className={styles.division}></div>
                            </div>
                        );
                    })
                }
                <button className={styles.loadMore} onClick={() => setVisibleLines(prev => prev + 100)}>
                    <h1 className={styles.text}> + </h1>
                </button>
            </div>
            {isSettingOpen && (
                <div className={styles.settings}>
                    <input
                        className={styles.searchLine}
                        type="number"
                        min="0"
                        max={data.length * 4}
                        placeholder="Endereço / Linha"
                        value={line}
                        onChange={(e) => setLine(e.target.value)}
                    />
                    <button className={styles.searchButton} onClick={searchLine}>Buscar</button>
                </div>
            )}
        </div>
    );
}
