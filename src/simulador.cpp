#include "simulador.hpp"

#include "components/Memory/BinaryUploader/BinaryUploader.hpp"
#include "../montador/montador.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <chrono>
#include <thread>

namespace rv32i {

Simulador::Simulador()
    : log_(SimulationLog::instance()),
      memory1_(4096, log_), memory2_(4096, log_), memory3_(4096, log_),
      uc1_(memory1_, log_), uc2_(memory2_, log_), uc3_(memory3_, log_),
      context_({memory1_, clock1_, uc1_, nullptr}, {memory2_, clock2_, uc2_, &uc2_.cache()},
               {memory3_, clock3_, uc3_, &uc3_.cache()}) {
    context_.assemblyCode =
        "# Programa Exemplo RV32I\n"
        "# Demonstração: ADDI, ADD, SW, LW, BEQ\n\n"
        "addi x1, x0, 10      # x1 = 10\n"
        "addi x2, x0, 20      # x2 = 20\n"
        "add  x3, x1, x2      # x3 = x1 + x2 = 30\n"
        "sw   x3, 0(x0)       # Grava 30 no endereço de RAM 0\n"
        "lw   x4, 0(x0)       # Lê palavra da RAM para x4\n"
        "beq  x3, x4, sucesso # Se x3 == x4, desvia para sucesso\n"
        "addi x5, x0, 99      # (Pulado pelo desvio)\n"
        "sucesso:\n"
        "addi x5, x0, 42      # x5 = 42 (Sucesso!)\n";
    uc1_.hardReset(0);
    uc2_.hardReset(0);
    uc3_.hardReset(0);
    publishStateSnapshot();
    worker_ = std::jthread([this](std::stop_token stopToken) { workerLoop(stopToken); });
}

Simulador::~Simulador() {
    turboEnabled_.store(false, std::memory_order_release);
    worker_.request_stop();
    workerWake_.notify_all();
    if (worker_.joinable()) worker_.join();
}

void Simulador::atualizar(Clock::TimePoint agora) {
    std::lock_guard lock(stateMutex_);
    if (turboEnabled_.load(std::memory_order_acquire)) return;
    if (context_.comparisonRunning) {
        if (comparisonLastUpdate_ == Clock::TimePoint{}) comparisonLastUpdate_ = agora;
        comparisonAccumulator_ += agora - comparisonLastUpdate_;
        comparisonLastUpdate_ = agora;
        const std::chrono::duration<double, std::milli> interval{context_.comparisonIntervalMs};
        std::size_t steps = 0;
        while (comparisonAccumulator_ >= interval && steps < 25 && context_.comparisonRunning) {
            comparisonAccumulator_ -= interval;
            avancarComparacaoSubciclo();
            ++steps;
        }
        if (steps == 25 && comparisonAccumulator_ >= interval)
            comparisonAccumulator_ = std::chrono::duration<double, std::milli>{
                std::fmod(comparisonAccumulator_.count(), interval.count())};
        publishStateSnapshot();
        return;
    }
    auto& machine = context_.selected();
    machine.clock.atualizar(machine.uc, agora);
    publishStateSnapshot();
}

bool Simulador::carregarBinario(const std::filesystem::path& path) {
    std::lock_guard lock(stateMutex_);
    const bool loaded = loadBinaryFile(context_, path);
    publishStateSnapshot();
    return loaded;
}

bool Simulador::prepararComparacao() {
    std::lock_guard lock(stateMutex_);
    setComparacaoEmExecucao(false);
    Montador assembler(context_.assemblyCode);
    if (const auto error = assembler.main()) {
        context_.comparisonError = *error;
        context_.comparisonProgramSize = 0;
        return false;
    }

    SimulationLog::instance().clear();
    memory1_.clearMemory();
    assembler.preencherMemoria(memory1_, log_);
    const auto words = memory1_.words();
    memory2_.clearMemory();
    memory3_.clearMemory();
    for (std::size_t index = 0; index < words.size(); ++index) {
        if (words[index] == 0) continue;
        const auto address = static_cast<std::uint32_t>(index * 4U);
        memory2_.writeMontador(address, words[index]);
        memory3_.writeMontador(address, words[index]);
    }
    context_.comparisonProgramSize = assembler.macroinstrucoes.size();
    context_.comparisonError.clear();
    context_.comparisonReady = true;
    context_.racePerformance.reset();
    uc2_.setCacheSize(static_cast<std::size_t>(context_.cacheSize));
    uc3_.setCacheSize(static_cast<std::size_t>(context_.cacheSize));
    for (auto& unit : context_.units) {
        unit.clock.resetClock();
        unit.uc.hardReset(0);
    }
    publishStateSnapshot();
    return true;
}

void Simulador::setComparacaoEmExecucao(bool running, Clock::TimePoint agora) {
    std::lock_guard lock(stateMutex_);
    context_.comparisonRunning = running;
    comparisonLastUpdate_ = agora;
    comparisonAccumulator_ = std::chrono::duration<double, std::milli>::zero();
}

void Simulador::avancarComparacaoSubciclo() {
    std::lock_guard lock(stateMutex_);
    for (std::size_t index = 0; index < context_.units.size(); ++index) {
        auto& lane = context_.units[index];
        const auto subcycle = lane.clock.getSC();
        const auto cycles = lane.clock.getCycles();
        if (subcycle == 1) {
            const auto address = lane.uc.registradores().readPC();
            const auto decoded = lane.uc.decodeInstruction(lane.memory.readWordUnsigned(address));
            context_.racePerformance.instructionStarted(static_cast<MacAtivo>(index), address,
                decoded.mnemonic, cycles);
        }
        lane.clock.subPulso(lane.uc);
        if (subcycle == 5) {
            context_.racePerformance.instructionFinished(static_cast<MacAtivo>(index),
                                                          lane.clock.getCycles());
        }
        context_.racePerformance.updateRuntime(static_cast<MacAtivo>(index),
            lane.clock.getCycles(), lane.uc.cacheHits(), lane.uc.cacheMisses(),
            lane.uc.cachePenaltyCycles());
    }
    publishStateSnapshot();
}

void Simulador::avancarComparacaoInstrucao() {
    std::lock_guard lock(stateMutex_);
    for (int subcycle = 0; subcycle < 5; ++subcycle) avancarComparacaoSubciclo();
}

void Simulador::resetarComparacao() {
    std::lock_guard lock(stateMutex_);
    setComparacaoEmExecucao(false);
    context_.racePerformance.reset();
    for (auto& lane : context_.units) {
        lane.clock.resetClock();
        lane.uc.hardReset(0);
    }
    publishStateSnapshot();
}

void Simulador::setTurboMode(bool enabled) {
    std::lock_guard lock(stateMutex_);
    if (enabled) {
        context_.selected().clock.pausar();
        context_.comparisonRunning = false;
        turboLane_.store(static_cast<std::size_t>(context_.macAtivo), std::memory_order_release);
    }
    context_.turboMode = enabled;
    turboEnabled_.store(enabled, std::memory_order_release);
    workerWake_.notify_all();
    publishStateSnapshot();
}

bool Simulador::turboMode() const noexcept {
    return turboEnabled_.load(std::memory_order_acquire);
}

std::shared_ptr<const std::array<Estado, 3>> Simulador::stateSnapshot() const noexcept {
    return publishedStates_.load(std::memory_order_acquire);
}

std::recursive_mutex& Simulador::stateMutex() noexcept { return stateMutex_; }

void Simulador::publishStateSnapshot() {
    auto snapshot = std::make_shared<std::array<Estado, 3>>();
    for (std::size_t index = 0; index < context_.units.size(); ++index) {
        const auto& lane = context_.units[index];
        (*snapshot)[index] = lane.uc.getEstado(lane.clock.getSC(), lane.clock.getCycles());
    }
    publishedStates_.store(std::move(snapshot), std::memory_order_release);
}

void Simulador::workerLoop(std::stop_token stopToken) {
    while (!stopToken.stop_requested()) {
        std::unique_lock waitLock(workerWaitMutex_);
        workerWake_.wait(waitLock, [this, &stopToken] {
            return stopToken.stop_requested() || turboEnabled_.load(std::memory_order_acquire);
        });
        waitLock.unlock();
        if (stopToken.stop_requested()) break;

        while (!stopToken.stop_requested() && turboEnabled_.load(std::memory_order_acquire)) {
            {
                std::lock_guard lock(stateMutex_);
                if (!turboEnabled_.load(std::memory_order_acquire)) break;
                auto& lane = context_.units[turboLane_.load(std::memory_order_acquire)];
                lane.clock.subPulso(lane.uc);
                context_.ipsMeter.update(lane.uc.getCompletedInstructions(), true,
                                         Clock::ClockSource::now());
                publishStateSnapshot();
            }
            std::this_thread::yield();
        }
    }
}

MacContext& Simulador::context() noexcept { return context_; }

} // namespace rv32i
