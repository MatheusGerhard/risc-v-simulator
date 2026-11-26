#include "performanceTracker.hpp"

#include <algorithm>

namespace rv32i {
namespace {
constexpr std::size_t indexOf(Processor processor) noexcept {
    return static_cast<std::size_t>(processor);
}
} // namespace

const char* PerformanceTracker::processorName(Processor processor) noexcept {
    switch (processor) {
    case Processor::Mac1: return "RV32I";
    case Processor::Mac2: return "RV32I (Cache D)";
    case Processor::Mac3: return "RV32I (Cache A)";
    }
    return "RV32I";
}

PerformanceTracker::PerformanceTracker() noexcept { reset(); }

void PerformanceTracker::reset() {
    std::lock_guard lock(mutex_);
    for (std::size_t i = 0; i < 3; ++i) {
        states_[i] = {};
        states_[i].processor = static_cast<Processor>(i);
    }
}

ProcessorPerformance& PerformanceTracker::mutableState(Processor processor) noexcept {
    return states_[indexOf(processor)];
}

ProcessorPerformance PerformanceTracker::state(Processor processor) const {
    std::lock_guard lock(mutex_);
    return states_[indexOf(processor)];
}

void PerformanceTracker::recalculate(ProcessorPerformance& state) noexcept {
    state.ipc = state.totalCycles ? static_cast<double>(state.completedInstructions) /
                                      static_cast<double>(state.totalCycles)
                                : 0.0;
    const auto accesses = state.cacheHits + state.cacheMisses;
    state.cacheHitRate = accesses ? std::optional<double>(static_cast<double>(state.cacheHits) /
                                                           static_cast<double>(accesses))
                                  : std::nullopt;
}

void PerformanceTracker::updateRuntime(Processor processor, std::uint64_t cycle,
                                       std::uint64_t cacheHits, std::uint64_t cacheMisses,
                                       std::uint64_t cyclePenaltyCycles) {
    std::lock_guard lock(mutex_);
    auto& current = mutableState(processor);
    current.cyclePenaltyCycles = cyclePenaltyCycles;
    current.totalCycles = std::max(current.totalCycles, cycle + cyclePenaltyCycles);
    current.cacheHits = cacheHits;
    current.cacheMisses = cacheMisses;
    recalculate(current);
}

void PerformanceTracker::instructionStarted(Processor processor, std::uint64_t cycle) {
    std::lock_guard lock(mutex_);
    auto& current = mutableState(processor);
    if (current.instructionOpen) {
        ++current.completedInstructions;
        if (current.currentInstruction) {
            auto finished = *current.currentInstruction;
            finished.endCycle = cycle;
            finished.cycles = cycle - finished.startCycle;
            finished.finished = true;
            current.instructions.push_back(std::move(finished));
            if (current.instructions.size() > 120) current.instructions.erase(current.instructions.begin());
        }
    }
    current.totalCycles = std::max(current.totalCycles, cycle);
    current.instructionOpen = true;
    current.currentInstruction = TrackedInstruction{0, 0, {}, cycle, 0, 0, false};
    recalculate(current);
}

void PerformanceTracker::instructionFinished(Processor processor, std::uint64_t cycle) {
    std::lock_guard lock(mutex_);
    auto& current = mutableState(processor);
    current.totalCycles = std::max(current.totalCycles, cycle);
    if (current.instructionOpen) {
        ++current.completedInstructions;
        current.instructionOpen = false;
        if (current.currentInstruction) {
            auto finished = *current.currentInstruction;
            finished.endCycle = cycle;
            finished.cycles = cycle - finished.startCycle;
            finished.finished = true;
            current.instructions.push_back(std::move(finished));
            if (current.instructions.size() > 120) current.instructions.erase(current.instructions.begin());
        }
    }
    recalculate(current);
}

void PerformanceTracker::instructionStarted(Processor processor, std::uint32_t address,
                                            std::string instructionText, std::uint64_t cycle) {
    std::lock_guard lock(mutex_);
    auto& current = mutableState(processor);
    if (current.instructionOpen) {
        ++current.completedInstructions;
        if (current.currentInstruction) {
            auto finished = *current.currentInstruction;
            finished.endCycle = cycle;
            finished.cycles = cycle - finished.startCycle;
            finished.finished = true;
            current.instructions.push_back(std::move(finished));
            if (current.instructions.size() > 120) current.instructions.erase(current.instructions.begin());
        }
    }
    current.totalCycles = std::max(current.totalCycles, cycle);
    current.instructionOpen = true;
    current.currentInstruction = TrackedInstruction{0, address, std::move(instructionText), cycle, 0, 0, false};
    if (current.currentInstruction) {
        current.currentInstruction->id = current.instructions.empty() ? 1 : current.instructions.back().id + 1;
    }
    recalculate(current);
}

std::vector<ProcessorPerformance> PerformanceTracker::snapshot() const {
    std::lock_guard lock(mutex_);
    return {states_[0], states_[1], states_[2]};
}

} // namespace rv32i
