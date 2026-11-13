import { useEffect, useState } from "react";
import memoria from "../../../../mac1/componentes/memory";
import styles from "./styles.module.css";

export default function StackMemoryDisplay({ uc, title = "Memória da Stack" }) {
    const [data, setData] = useState([...memoria.data]);
    const [sp, setSp] = useState(0);
    const [displayMode, setDisplayMode] = useState("hex"); // "hex", "bin", "dec"

    useEffect(() => {
        const update = () => {
            setData([...memoria.data]);
            if (uc && uc.regs && typeof uc.regs.readUInt === "function") {
                setSp(uc.regs.readUInt(2));
            } else if (uc && uc.regs && typeof uc.regs.readInt === "function") {
                setSp(uc.regs.readInt(2) >>> 0);
            }
        };

        update();
        const interval = setInterval(update, 100);
        return () => clearInterval(interval);
    }, [uc]);

    function changeBase() {
        if (displayMode === "hex") setDisplayMode("bin");
        else if (displayMode === "bin") setDisplayMode("dec");
        else setDisplayMode("hex");
    }

    function formatValue(value) {
        const binaryValue = String(value || "0").padStart(32, "0").slice(-32);

        if (displayMode === "bin") {
            return binaryValue;
        }

        const uintVal = parseInt(binaryValue, 2) >>> 0;

        if (displayMode === "hex") {
            return "0x" + uintVal.toString(16).padStart(8, "0").toUpperCase();
        }

        // Decimal com sinal
        return parseInt(binaryValue, 2) | 0;
    }

    const totalWords = data.length;
    const totalBytes = memoria.size || totalWords * 4;
    const startWordIndex = Math.max(0, Math.floor(sp / 4));
    const spHex = "0x" + (sp >>> 0).toString(16).padStart(4, "0").toUpperCase();
    const endHex = "0x" + Math.max(0, totalBytes - 1).toString(16).padStart(4, "0").toUpperCase();

    // Stack vazia se sp estiver no topo da memória ou acima
    const isStackEmpty = totalBytes > 0 && sp >= totalBytes;

    // Fatiar do SP até o final da memória
    const stackItems = [];
    if (!isStackEmpty && startWordIndex < totalWords) {
        for (let i = startWordIndex; i < totalWords; i++) {
            stackItems.push({
                wordIndex: i,
                byteAddr: i * 4,
                hexAddr: "0x" + (i * 4).toString(16).padStart(4, "0").toUpperCase(),
                value: data[i],
                isSp: i === startWordIndex
            });
        }
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
                </div>

                <div className={styles.titleArea}>
                    <div className={styles.title}>{title}</div>
                </div>

                <div
                    className={styles.spBadgeHeader}
                    title={`Stack Pointer (x2 / sp): ${spHex} (${sp} dec)\nFim da memória: ${endHex}`}
                >
                    SP: {spHex}
                </div>
            </div>

            <div className={styles.container}>
                {isStackEmpty ? (
                    <div className={styles.emptyState}>
                        <div className={styles.emptyTitle}>Pilha Vazia</div>
                        <div className={styles.emptyDesc}>
                            O Stack Pointer aponta para o topo da memória ({spHex} ≥ {endHex}).
                            <br />
                            Nenhum elemento alocado na pilha no momento.
                        </div>
                    </div>
                ) : stackItems.length === 0 ? (
                    <div className={styles.emptyState}>
                        <div className={styles.emptyTitle}>Fora dos Limites</div>
                        <div className={styles.emptyDesc}>
                            SP ({spHex}) está além da faixa de memória mapeada (0x0000 - {endHex}).
                        </div>
                    </div>
                ) : (
                    stackItems.map((item) => (
                        <div key={item.wordIndex}>
                            <div className={`${styles.addressRow} ${item.isSp ? styles.spActiveRow : ""}`}>
                                <div className={styles.indexCol}>
                                    <div
                                        className={styles.index}
                                        title={`Endereço de Byte: ${item.byteAddr} (Palavra ${item.wordIndex})`}
                                    >
                                        {item.hexAddr}:
                                    </div>
                                    {item.isSp && (
                                        <span className={styles.spIndicator} title="Topo atual da pilha (SP)">
                                            SP
                                        </span>
                                    )}
                                </div>
                                <div className={`${styles.value} ${item.isSp ? styles.spActiveValue : ""}`}>
                                    {formatValue(item.value)}
                                </div>
                            </div>
                            <div className={styles.division}></div>
                        </div>
                    ))
                )}
            </div>
        </div>
    );
}
