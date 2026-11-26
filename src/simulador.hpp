#pragma once

#include "context/MacContext.hpp"
#include "../mac2/controlUnit.hpp"
#include "../mac3/controlUnit.hpp"

#include <filesystem>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace rv32i {

class Simulador {
public:
    Simulador();
    ~Simulador();
    void atualizar(Clock::TimePoint agora = Clock::ClockSource::now());
    [[nodiscard]] bool carregarBinario(const std::filesystem::path& path);
    [[nodiscard]] bool prepararComparacao();
    void setComparacaoEmExecucao(bool running, Clock::TimePoint agora = Clock::ClockSource::now());
    void avancarComparacaoSubciclo();
    void avancarComparacaoInstrucao();
    void resetarComparacao();
    void setTurboMode(bool enabled);
    [[nodiscard]] bool turboMode() const noexcept;
    [[nodiscard]] std::shared_ptr<const std::array<Estado, 3>> stateSnapshot() const noexcept;
    [[nodiscard]] std::recursive_mutex& stateMutex() noexcept;
    [[nodiscard]] MacContext& context() noexcept;

private:
    void workerLoop(std::stop_token stopToken);
    void publishStateSnapshot();
    SimulationLog& log_;
    Memory memory1_;
    Memory memory2_;
    Memory memory3_;
    Clock clock1_;
    Clock clock2_;
    Clock clock3_;
    ControlUnit uc1_;
    mac2::ControlUnit uc2_;
    mac3::ControlUnit uc3_;
    MacContext context_;
    Clock::TimePoint comparisonLastUpdate_{};
    std::chrono::duration<double, std::milli> comparisonAccumulator_{};
    std::recursive_mutex stateMutex_;
    std::atomic<bool> turboEnabled_{};
    std::atomic<std::size_t> turboLane_{};
    std::atomic<std::shared_ptr<const std::array<Estado, 3>>> publishedStates_;
    std::mutex workerWaitMutex_;
    std::condition_variable workerWake_;
    std::jthread worker_;
};

} // namespace rv32i
