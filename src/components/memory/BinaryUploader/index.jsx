import { useRef, useState } from "react";
import styles from "./styles.module.css";

export default function BinaryUploader({ onLoadBinary }) {
    const fileInputRef = useRef(null);
    const [baseAddress, setBaseAddress] = useState("0x00000000");
    const [loadedInfo, setLoadedInfo] = useState(null);
    const [showHelp, setShowHelp] = useState(false);

    function handleClick() {
        if (fileInputRef.current) {
            fileInputRef.current.value = "";
            fileInputRef.current.click();
        }
    }

    function handleFileChange(event) {
        const file = event.target.files?.[0];
        if (!file) return;

        const reader = new FileReader();

        reader.onload = (e) => {
            try {
                const arrayBuffer = e.target?.result;
                if (!arrayBuffer) {
                    throw new Error("Arquivo vazio ou ilegível.");
                }

                const byteArray = new Uint8Array(arrayBuffer);
                const bytesLoaded = onLoadBinary(byteArray, baseAddress);

                setLoadedInfo({
                    name: file.name,
                    size: bytesLoaded,
                    address: baseAddress,
                });
            } catch (err) {
                alert(`Erro ao carregar o arquivo .bin: ${err.message}`);
            }
        };

        reader.onerror = () => {
            alert("Erro ao ler o arquivo selecionado no navegador.");
        };

        reader.readAsArrayBuffer(file);
    }

    return (
        <div className={styles.uploadCard}>
            <div className={styles.header}>
                <span className={styles.cardTitle}>Importar Binário Real (.bin)</span>
                <button
                    className={styles.helpButton}
                    onClick={() => setShowHelp((prev) => !prev)}
                    title="Instruções de compilação GCC"
                >
                    {showHelp ? "✕" : "ℹ Toolchain"}
                </button>
            </div>

            {showHelp && (
                <div className={styles.helpBox}>
                    <p className={styles.helpText}>
                        <strong>Aviso:</strong> O arquivo deve ser um <em>Raw Flat Binary</em> (código de máquina puro, bare-metal), <strong>não</strong> um arquivo <code>.elf</code>.
                    </p>
                    <p className={styles.helpCommandTitle}>Comando padrão de extração no GCC:</p>
                    <pre className={styles.commandCode}>
                        riscv64-unknown-elf-objcopy -O binary programa.elf programa.bin
                    </pre>
                </div>
            )}

            <div className={styles.controlsRow}>
                <div className={styles.addressWrapper}>
                    <label className={styles.addressLabel}>Base:</label>
                    <input
                        type="text"
                        className={styles.addressInput}
                        value={baseAddress}
                        onChange={(e) => setBaseAddress(e.target.value)}
                        placeholder="0x00000000"
                        title="Endereço base na memória RAM (Hex ou Decimal)"
                    />
                </div>

                <input
                    type="file"
                    ref={fileInputRef}
                    accept=".bin"
                    style={{ display: "none" }}
                    onChange={handleFileChange}
                />

                <button
                    className={styles.uploadBtn}
                    onClick={handleClick}
                    title="Carregar arquivo .bin gerado por objcopy"
                >
                    📂 Upload .bin
                </button>
            </div>

            {loadedInfo && (
                <div className={styles.statusSuccess}>
                    ✓ <strong>{loadedInfo.name}</strong> ({loadedInfo.size} bytes) carregado em <code>{loadedInfo.address}</code>. PC inicializado!
                </div>
            )}
        </div>
    );
}
