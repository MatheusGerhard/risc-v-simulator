#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace rv32i {

enum class MacAtivo { Mac1, Mac2, Mac3, RV32I = Mac1, RV32I_C1 = Mac2, RV32I_C2 = Mac3 };
using Processor = MacAtivo;

struct TrackedInstruction {
    std::uint64_t id{};
    std::uint32_t address{};
    std::string instructionText;
    std::uint64_t startCycle{};
    std::uint64_t endCycle{};
    std::uint64_t cycles{};
    bool finished{};
};

struct ProcessorPerformance {
    Processor processor{};
    std::uint64_t totalCycles{};
    std::uint64_t completedInstructions{};
    double ipc{};
    std::uint64_t cacheHits{};
    std::uint64_t cacheMisses{};
    std::uint64_t cyclePenaltyCycles{};
    bool instructionOpen{};
    std::optional<double> cacheHitRate;
    std::vector<TrackedInstruction> instructions;
    std::optional<TrackedInstruction> currentInstruction;
};

class PerformanceTracker {
public:
    PerformanceTracker() noexcept;
    static const char* processorName(Processor processor) noexcept;
    void reset();
    void updateRuntime(Processor processor, std::uint64_t cycle,
                       std::uint64_t cacheHits = 0, std::uint64_t cacheMisses = 0,
                       std::uint64_t cyclePenaltyCycles = 0);
    void instructionStarted(Processor processor, std::uint64_t cycle);
    void instructionFinished(Processor processor, std::uint64_t cycle);
    void instructionStarted(Processor processor, std::uint32_t address,
                            std::string instructionText, std::uint64_t cycle);
    [[nodiscard]] ProcessorPerformance state(Processor processor) const;
    [[nodiscard]] std::vector<ProcessorPerformance> snapshot() const;

private:
    ProcessorPerformance& mutableState(Processor processor) noexcept;
    static void recalculate(ProcessorPerformance& state) noexcept;
    mutable std::mutex mutex_;
    ProcessorPerformance states_[3]{};
};

} // namespace rv32i
