// Hook customizado para monitoramento de Instruções Por Segundo (IPS) em tempo real
// Mede a taxa de execução wall-clock (tempo real da máquina hospedeira)
// Apenas atualiza o estado React 1x por segundo via setInterval, evitando re-renderizações desnecessárias.

import { useState, useEffect, useRef } from "react";

export function formatIPS(ips, isRunning = true) {
    if (!isRunning || !ips || ips <= 0) {
        return "0 IPS";
    }

    if (ips >= 1_000_000) {
        return `${(ips / 1_000_000).toFixed(1)}M IPS`;
    }

    if (ips >= 1_000) {
        return `${(ips / 1_000).toFixed(1)}k IPS`;
    }

    if (ips < 10) {
        return `${ips.toFixed(1)} IPS`;
    }

    return `${Math.round(ips)} IPS`;
}

export function useIPS(controlUnit, isRunning) {
    const [ips, setIps] = useState(0);
    const lastTimeRef = useRef(performance.now());
    const lastCountRef = useRef(0);

    useEffect(() => {
        // Inicializa ou sincroniza com o valor atual do processador
        lastTimeRef.current = performance.now();
        lastCountRef.current = controlUnit?.getCompletedInstructions?.() ?? 0;

        if (!isRunning) {
            setIps(0);
        }

        const intervalId = setInterval(() => {
            const now = performance.now();
            const currentCount = controlUnit?.getCompletedInstructions?.() ?? 0;
            const elapsedSeconds = (now - lastTimeRef.current) / 1000;

            if (elapsedSeconds > 0 && isRunning) {
                const deltaInstructions = currentCount - lastCountRef.current;
                const currentIPS = deltaInstructions > 0 ? (deltaInstructions / elapsedSeconds) : 0;
                setIps(currentIPS);
            } else {
                setIps(0);
            }

            lastTimeRef.current = now;
            lastCountRef.current = currentCount;
        }, 1000);

        return () => clearInterval(intervalId);
    }, [controlUnit, isRunning]);

    return {
        ips,
        formattedIPS: formatIPS(ips, isRunning),
    };
}

export default useIPS;
