import styles from './styles.module.css';
import { useEffect, useState } from 'react';
import { uc1, uc2, uc3 } from "../../../testes/teste3";
import ALU from './ALU';
import { useMac } from '../../context/MacContext';
import { ABI_NAMES } from '../../../mac1/componentes/registers';

const DEFAULT_CACHE_SIZE = 3;
const CONTROL_UNITS = {
    mac1: uc1,
    mac2: uc2,
    mac3: uc3,
};

function normalizeCacheSize(size) {
    const numericSize = Number.parseInt(size, 10);
    return Number.isFinite(numericSize) && numericSize >= 1
        ? numericSize
        : DEFAULT_CACHE_SIZE;
}

function readCacheSnapshot(controlUnit) {
    return controlUnit?.cache?.getSnapshot?.() ?? null;
}

function formatCacheCell(value) {
    return value === null || value === undefined || value === "" ? "-" : value;
}

const estadoInicial = {
    subciclo: 1, ciclos: 0,
    stage: 'FETCH',
    mir: 'FETCH', mpc: 'NOP',
    mar: '0x00000000', mbr: '0x00000000',
    aluRes: '0x00000000', aluZ: 1, aluN: 0,
    latA: '0x00000000', latB: '0x00000000',
    amux: '0x00000000', shifter: '0x00000000',
    mmux: 'PC+4', msl: 'NOT TAKEN',
    pc: '0x00000000',
    opcodeName: 'NOP',
    format: 'I-Type',
    rd: 'x0 (zero)',
    rs1: 'x0 (zero)',
    rs2: 'x0 (zero)',
    imm: '0x00000000',
    registersList: []
};

function COMPONENTE({ label, value, className }) {
    return (
        <div className={`${styles.componente} ${className || ""}`}>
            <header className={styles.label}>
                {label}
            </header>
            <div className={styles.value}>
                <h1>{value}</h1>
            </div>
        </div>
    );
}

function BancoDeRegistradores({ estado }) {
    const [displayBase, setDisplayBase] = useState("hex"); // "hex", "dec", "bin"

    // Se temos a lista rica de registradores com metadados
    const regs = estado.registersList && estado.registersList.length === 32
        ? estado.registersList
        : Array.from({ length: 32 }, (_, i) => ({
            index: i,
            name: `x${i}`,
            abi: ABI_NAMES[i],
            label: `x${i} (${ABI_NAMES[i]})`,
            bin: estado[`x${i}`] || "00000000000000000000000000000000",
            hex: estado[`x${i}`] ? `0x${parseInt(estado[`x${i}`], 2).toString(16).padStart(8, '0').toUpperCase()}` : "0x00000000",
            dec: estado[`x${i}`] ? parseInt(estado[`x${i}`], 2) | 0 : 0
        }));

    function getRegDisplayValue(reg) {
        if (displayBase === "hex") return reg.hex;
        if (displayBase === "dec") return reg.dec;
        return reg.bin.slice(-16); // exibe os 16 bits inferiores no modo bin para caber melhor na célula
    }

    function toggleBase() {
        if (displayBase === "hex") setDisplayBase("dec");
        else if (displayBase === "dec") setDisplayBase("bin");
        else setDisplayBase("hex");
    }

    return (
        <div className={styles.banco}>
            <div className={styles.tituloBanco}>
                <span>Registradores RV32I (x0 - x31)</span>
                <button
                    className={styles.baseToggleBtn}
                    onClick={toggleBase}
                    title="Alternar entre Hexadecimal, Decimal e Binário"
                >
                    Formato: {displayBase.toUpperCase()}
                </button>
            </div>

            <div className={styles.registradoresGrid}>
                {regs.map((reg) => (
                    <div key={reg.index} className={styles.regItem}>
                        <div className={styles.regLabel}>
                            {reg.name} ({reg.abi})
                        </div>
                        <div className={styles.regValue}>
                            {getRegDisplayValue(reg)}
                        </div>
                    </div>
                ))}
            </div>
        </div>
    );
}

function CacheContent({ snapshot }) {
    const hasCache = Array.isArray(snapshot) && snapshot.length > 0;

    return (
        <section className={styles.cachePanel}>
            <h2>Conteúdo da Cache</h2>

            {!hasCache ? (
                <div className={styles.cacheUnavailable}>Modo Direto RV32I (Cache desativada ou não configurada)</div>
            ) : (
                <div className={styles.cacheTableWrapper}>
                    <table className={styles.cacheTable}>
                        <thead>
                            <tr>
                                <th>Linha</th>
                                <th>V</th>
                                <th>Endereço</th>
                                <th>Valor</th>
                            </tr>
                        </thead>
                        <tbody>
                            {snapshot.map((line) => (
                                <tr key={line.index}>
                                    <td>{line.index}</td>
                                    <td>{line.valid ? "1" : "0"}</td>
                                    <td>{formatCacheCell(line.addressRange ?? line.address)}</td>
                                    <td>{formatCacheCell(line.value)}</td>
                                </tr>
                            ))}
                        </tbody>
                    </table>
                </div>
            )}
        </section>
    );
}

export default function Simulation() {
    const [estado, setEstado] = useState(estadoInicial);
    const [cacheSize, setCacheSize] = useState(uc2.cacheSize ?? DEFAULT_CACHE_SIZE);
    const [cacheSnapshot, setCacheSnapshot] = useState(() =>
        readCacheSnapshot(CONTROL_UNITS.mac1),
    );
    const { activeMac, setActiveMac } = useMac();

    useEffect(() => {
        const uc = CONTROL_UNITS[activeMac];

        if (!uc) return;

        setCacheSnapshot(readCacheSnapshot(uc));

        uc.setCallback((novoEstado) => {
            setEstado(novoEstado);
            setCacheSnapshot(readCacheSnapshot(uc));
        });
    }, [activeMac]);

    useEffect(() => {
        if (uc2.setCacheSize) uc2.setCacheSize(cacheSize);
        if (uc3.setCacheSize) uc3.setCacheSize(cacheSize);
    }, [cacheSize]);

    useEffect(() => {
        setCacheSnapshot(readCacheSnapshot(CONTROL_UNITS[activeMac]));
    }, [activeMac, cacheSize]);

    // Polling de sincronização
    useEffect(() => {
        const uc = CONTROL_UNITS[activeMac];
        if (!uc) return;
        const interval = setInterval(() => {
            setCacheSnapshot(readCacheSnapshot(uc));
        }, 100);
        return () => clearInterval(interval);
    }, [activeMac]);

    return (
        <div className={styles.simBox}>
            <div className={styles.selectProcesser}>
                <button
                    onClick={() => setActiveMac("mac1")}
                    className={`${styles.botao} ${activeMac === "mac1" ? styles.ativo : ""}`}
                >
                    RV32I (Datapath)
                </button>

                <button
                    onClick={() => setActiveMac("mac2")}
                    className={`${styles.botao} ${activeMac === "mac2" ? styles.ativo : ""}`}
                >
                    RV32I (Cache D)
                </button>

                <button
                    onClick={() => setActiveMac("mac3")}
                    className={`${styles.botao} ${activeMac === "mac3" ? styles.ativo : ""}`}
                >
                    RV32I (Cache A)
                </button>
                <label className={styles.cacheControl}>
                    <span>Cache</span>
                    <input
                        min="1"
                        step="1"
                        type="number"
                        value={cacheSize}
                        className={styles.cacheControlInput}
                        onChange={(event) =>
                            setCacheSize(normalizeCacheSize(event.target.value))
                        }
                    />
                </label>
            </div>

            <div className={styles.simulation}>
                <BancoDeRegistradores estado={estado}/>

                <COMPONENTE
                    label="Clock & FSM"
                    value={`Ciclos: ${estado.ciclos} | Subciclo: ${estado.subciclo}`}
                    className={styles.clock}
                />

                <COMPONENTE
                    label="Etapa Atual"
                    value={estado.stage || estado.mir}
                    className={styles.decA}
                />

                <COMPONENTE
                    label="Mnemônico"
                    value={estado.opcodeName || estado.mpc}
                    className={styles.decB}
                />

                <COMPONENTE
                    label="Formato / Imm"
                    value={`${estado.format || "-"} | ${estado.imm || "0"}`}
                    className={styles.decC}
                />

                <COMPONENTE
                    label="Operação FSM"
                    value={estado.micro || estado.mir}
                    className={styles.mir}
                />

                <COMPONENTE
                    label="PC"
                    value={estado.pc}
                    className={styles.mpc}
                />

                <COMPONENTE
                    label="Desvio (Branch)"
                    value={estado.msl}
                    className={styles.msl}
                />

                <COMPONENTE
                    label="Próximo PC"
                    value={estado.mmux}
                    className={styles.mmux}
                />

                <COMPONENTE
                    label="MAR (Endereço RAM)"
                    value={estado.mar}
                    className={styles.mar}
                />

                <COMPONENTE
                    label="MBR / MDR (Dados RAM)"
                    value={estado.mbr}
                    className={styles.mbr}
                />

                <COMPONENTE
                    label="RS1 (Dado A)"
                    value={estado.latA}
                    className={styles.latchA}
                />

                <COMPONENTE
                    label="RS2 (Dado B)"
                    value={estado.latB}
                    className={styles.latchB}
                />

                <COMPONENTE
                    label="Operando A (ALU)"
                    value={estado.amux}
                    className={styles.amux}
                />

                <ALU
                    label="ALU RV32I"
                    value={`${estado.aluRes} (Z=${estado.aluZ} N=${estado.aluN})`}
                    className={styles.alu}
                />

                <COMPONENTE
                    label="ALU Out / WB"
                    value={estado.shifter}
                    className={styles.shifter}
                />
            </div>

            <CacheContent snapshot={cacheSnapshot} />
        </div>
    );
}
