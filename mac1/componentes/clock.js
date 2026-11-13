// Clock.js do Simulador RV32I
// Controla os pulsos de clock e os 5 subciclos do ciclo de instrução clássico:
// 1: Fetch (IF)
// 2: Decode (ID)
// 3: Execute (EX)
// 4: Memory (MEM)
// 5: Write-back (WB)

class Clock {
    constructor() {
        this.subciclo = 1;
        this.totalCiclos = 0;
        this.intervaloMs = 1000;
        this.emExecucao = false;
        this.timerId = null;
        this.halt = 0;
        this.onCicloChange = null;
    }

    setVelocidade(ms) {
        const parsed = Number(ms);
        this.intervaloMs = !isNaN(parsed) && parsed > 0 ? parsed : 1000;
        if (this.emExecucao) {
            this.pausar();
            this.iniciar(this.currentControlUnit);
        }
    }

    subPulso(controlUnit) {
        this.currentControlUnit = controlUnit;
        if (this.halt) {
            console.log("Simulador pausado ou em HALT.");
            return;
        }

        const sucesso = controlUnit.rodarCiclo(this.subciclo, this.totalCiclos);
        if (sucesso === false) {
            console.log("Execução interrompida.");
            this.pausar();
            return;
        }

        this.subciclo++;
        if (this.subciclo > 5) {
            this.subciclo = 1;
            this.totalCiclos++;
            if (this.onCicloChange) this.onCicloChange(this.totalCiclos);
        }
    }

    pulso(controlUnit) {
        // Executa uma instrução completa (5 subciclos)
        for (let i = 1; i <= 5; i++) {
            this.subPulso(controlUnit);
        }
        if (this.onCicloChange) this.onCicloChange(this.totalCiclos);
    }

    iniciar(controlUnit) {
        if (this.emExecucao) return;
        this.emExecucao = true;
        this.currentControlUnit = controlUnit;

        this.timerId = setInterval(() => {
            if (!this.emExecucao) {
                clearInterval(this.timerId);
                return;
            }
            this.subPulso(controlUnit);
        }, this.intervaloMs);
    }

    pausar() {
        this.emExecucao = false;
        if (this.timerId) {
            clearInterval(this.timerId);
            this.timerId = null;
        }
    }

    getCycles() {
        return this.totalCiclos;
    }

    getSC() {
        return this.subciclo;
    }

    reset() {
        this.pausar();
        this.totalCiclos = 0;
        this.subciclo = 1;
        if (this.onCicloChange) this.onCicloChange(this.totalCiclos);
    }

    resetClock() {
        this.pausar();
        this.subciclo = 1;
        this.totalCiclos = 0;
        this.emExecucao = false;
        if (this.onCicloChange) this.onCicloChange(this.totalCiclos);
    }
}

export default Clock;
