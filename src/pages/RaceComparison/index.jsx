import { useEffect, useRef, useState } from "react";
import ControlUnit1 from "../../../mac1/controlUnit";
import ControlUnit2 from "../../../mac2/controlUnit";
import ControlUnit3 from "../../../mac3/controlUnit";
import { Memory } from "../../../mac1/componentes/memory";
import Montador from "../../../montador/montador";
import {
    clearSimulationLog,
    registerMacroInstructions,
} from "../../services/simulationLog";
import {
    getPerformanceSnapshot,
    subscribeToPerformanceTracker,
    updateProcessorRuntime,
} from "../../services/performanceTracker";
import ProcessorTrack from "./ProcessorTrack";
import styles from "./styles.module.css";

const DEFAULT_PROGRAM = `addi x1, x0, 10
addi x2, x0, 20
add x3, x1, x2
sw x3, 0(x0)
lw x4, 0(x0)
beq x3, x4, fim
addi x5, x0, 1
fim:
addi x5, x0, 42`;
const DEFAULT_CACHE_SIZE = 3;

const PROCESSORS = [
    { name: "RV32I", ControlUnit: ControlUnit1, accent: "#38bdf8" },
    { name: "RV32I-C1", ControlUnit: ControlUnit2, accent: "#22c55e", usesCache: true },
    { name: "RV32I-C2", ControlUnit: ControlUnit3, accent: "#f59e0b", usesCache: true },
];

function normalizeCacheSize(size) {
    const numericSize = Number.parseInt(size, 10);
    return Number.isFinite(numericSize) && numericSize >= 1
        ? numericSize
        : DEFAULT_CACHE_SIZE;
}

function createEmptyCacheSnapshot(size) {
    return Array.from({ length: normalizeCacheSize(size) }, (_, index) => ({
        index,
        valid: false,
        address: null,
        addressRange: null,
        value: null,
        values: [],
    }));
}

function formatInstruction(instruction) {
    if (!instruction.opcode) {
        return null;
    }

    return instruction.operando === null || instruction.operando === undefined
        ? instruction.opcode
        : `${instruction.opcode} ${instruction.operando}`;
}

function buildProgram(assemblyCode) {
    const montador = new Montador(assemblyCode);
    const result = montador.main();

    if (result) {
        throw new Error(result);
    }

    if (result === null) {
        throw new Error("Erro na montagem do programa.");
    }

    registerMacroInstructions(montador.macroinstrucoes);

    const memory = new Memory();
    memory.preencheInstrucoes(montador.microinstrucoes, montador.macroinstrucoes);

    return {
        baseMemory: [...memory.data],
        instructions: montador.macroinstrucoes
            .filter((instruction) => instruction.opcode !== null)
            .map((instruction) => ({
                address: Number(instruction.rotulo_index),
                text: formatInstruction(instruction),
            })),
    };
}

function cloneMemory(baseMemory) {
    const memory = new Memory(baseMemory.length);
    memory.data = [...baseMemory];
    return memory;
}

function bindMemory(controlUnit, memory) {
    controlUnit.ram = memory;

    if (controlUnit.cache) {
        controlUnit.cache.ram = memory;
    }
}

function createLane(config, baseMemory, cacheSize) {
    const controlUnit = config.usesCache
        ? new config.ControlUnit(cacheSize)
        : new config.ControlUnit();
    const memory = cloneMemory(baseMemory);

    bindMemory(controlUnit, memory);

    return {
        processor: config.name,
        controlUnit,
        memory,
        subCycle: 1,
        totalCycles: 0,
        halt: 0,
    };
}

function getEffectiveCycle(lane) {
    if (lane.processor === "MAC-3") {
        return lane.totalCycles * 4 + lane.subCycle - 1;
    }

    return lane.totalCycles;
}

function updateLaneRuntime(lane) {
    updateProcessorRuntime({
        processor: lane.processor,
        cycle: getEffectiveCycle(lane),
        cacheHits: lane.controlUnit.hits ?? 0,
        cacheMisses: lane.controlUnit.misses ?? 0,
    });
}

function stepLane(lane) {
    bindMemory(lane.controlUnit, lane.memory);

    if (lane.halt) {
        lane.halt = 0;
        lane.totalCycles++;
        updateLaneRuntime(lane);
        return;
    }

    const shouldContinue = lane.controlUnit.rodarCiclo(lane.subCycle, lane.totalCycles);

    if (shouldContinue === false) {
        lane.halt = 1;
        updateLaneRuntime(lane);
        return;
    }

    lane.subCycle++;

    if (lane.subCycle > 4) {
        lane.subCycle = 1;
        lane.totalCycles++;
    }

    updateLaneRuntime(lane);
}

export default function RaceComparison() {
    const [assemblyCode, setAssemblyCode] = useState(DEFAULT_PROGRAM);
    const [speed, setSpeed] = useState(250);
    const [cacheSize, setCacheSize] = useState(DEFAULT_CACHE_SIZE);
    const [isReady, setIsReady] = useState(false);
    const [isRunning, setIsRunning] = useState(false);
    const [error, setError] = useState("");
    const [programSize, setProgramSize] = useState(0);
    const [snapshot, setSnapshot] = useState(getPerformanceSnapshot());
    const lanesRef = useRef([]);
    const timerRef = useRef(null);

    useEffect(() => {
        return subscribeToPerformanceTracker(setSnapshot);
    }, []);

    useEffect(() => {
        return () => {
            if (timerRef.current) {
                clearInterval(timerRef.current);
            }
        };
    }, []);

    function pauseRace() {
        if (timerRef.current) {
            clearInterval(timerRef.current);
            timerRef.current = null;
        }

        setIsRunning(false);
    }

    function clearRace() {
        pauseRace();
        clearSimulationLog();
        lanesRef.current = [];
        setIsRunning(false);
        setIsReady(false);
        setError("");
        setProgramSize(0);
    }

    function handleCacheSizeChange(event) {
        pauseRace();
        setCacheSize(normalizeCacheSize(event.target.value));
        setIsReady(false);
    }

    function prepareRace() {
        pauseRace();

        try {
            clearSimulationLog();
            const program = buildProgram(assemblyCode);
            lanesRef.current = PROCESSORS.map((processor) =>
                createLane(processor, program.baseMemory, cacheSize),
            );
            lanesRef.current.forEach(updateLaneRuntime);
            setProgramSize(program.instructions.length);
            setError("");
            setIsReady(true);
            return lanesRef.current;
        } catch (currentError) {
            lanesRef.current = [];
            setProgramSize(0);
            setIsReady(false);
            setError(currentError.message);
            return null;
        }
    }

    function startRace() {
        const lanes = isReady ? lanesRef.current : prepareRace();

        if (!lanes || lanes.length === 0 || timerRef.current) {
            return;
        }

        setIsRunning(true);
        timerRef.current = setInterval(() => {
            lanesRef.current.forEach(stepLane);
        }, Number(speed) || 250);
    }

    function stepRace() {
        const lanes = isReady ? lanesRef.current : prepareRace();

        if (!lanes || lanes.length === 0) {
            return;
        }

        lanes.forEach(stepLane);
    }

    const [cacheSnapshots, setCacheSnapshots] = useState({});

    useEffect(() => {
        const interval = setInterval(() => {
            if (!isReady) return;
            const updated = {};
            lanesRef.current.forEach((lane) => {
                if (lane.controlUnit.cache) {
                    updated[lane.processor] = lane.controlUnit.cache.getSnapshot();
                }
            });
            setCacheSnapshots(updated);
        }, 100);
        return () => clearInterval(interval);
    }, [isReady]);

    const snapshotByProcessor = new Map(
        snapshot.map((processorState) => [processorState.processor, processorState]),
    );
    const laneByProcessor = new Map(
        isReady ? lanesRef.current.map((lane) => [lane.processor, lane]) : [],
    );
    const emptyCacheSnapshot = createEmptyCacheSnapshot(cacheSize);

    return (
        <section className={styles.page}>
            <div className={styles.controlBand}>
                <div className={styles.editor}>
                    <div className={styles.editorHeader}>
                        <h1>Comparacao MAC</h1>
                        <span>{programSize} instrucoes</span>
                    </div>
                    <textarea
                        value={assemblyCode}
                        onChange={(event) => {
                            setAssemblyCode(event.target.value);
                            setIsReady(false);
                        }}
                        className={styles.textarea}
                    />
                </div>

                <div className={styles.controls}>
                    <button onClick={prepareRace}>Montar</button>
                    <button
                        className={isRunning ? styles.activeButton : ""}
                        onClick={startRace}
                    >
                        Run
                    </button>
                    <button onClick={pauseRace}>Pause</button>
                    <button onClick={stepRace}>Step</button>
                    <button onClick={clearRace}>Clear</button>
                    <label className={styles.speedControl}>
                        <span>ms</span>
                        <input
                            min="25"
                            type="number"
                            value={speed}
                            onChange={(event) => setSpeed(event.target.value)}
                        />
                    </label>
                    <label className={styles.cacheControl}>
                        <span>Cache</span>
                        <input
                            min="1"
                            step="1"
                            type="number"
                            value={cacheSize}
                            onChange={handleCacheSizeChange}
                        />
                    </label>
                    {error && <div className={styles.error}>{error}</div>}
                </div>
            </div>

            <div className={styles.raceBoard}>
                {PROCESSORS.map((processor) => (
                    <ProcessorTrack
                        accent={processor.accent}
                        cacheSnapshot={
                            processor.usesCache
                                ? cacheSnapshots[processor.name] ?? emptyCacheSnapshot
                                : null
                        }
                        hasCache={Boolean(processor.usesCache)}
                        key={processor.name}
                        state={snapshotByProcessor.get(processor.name)}
                    />
                ))}
            </div>
        </section>
    );
}