#pragma once

#include "../../mac1/componentes/clock.hpp"
#include "../../mac1/componentes/memory.hpp"
#include "../../mac1/controlUnit.hpp"
#include "../../mac2/componentes/cache.hpp"
#include "../hooks/useIPS.hpp"
#include "../services/performanceTracker.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <string>

namespace rv32i {

enum class MemoryDisplayMode { Hex, Bin, Dec };
enum class RegisterDisplayMode { Hex, Dec, Bin };

struct MemoryViewState {
    explicit MemoryViewState(bool isOpen = false) : open(isOpen) {}
    bool open{};
    MemoryDisplayMode mode{MemoryDisplayMode::Hex};
    std::string addressQuery;
    bool scrollRequested{};
    std::size_t targetLine{};
};

struct MacUnitRef {
    Memory& memory;
    Clock& clock;
    ControlUnit& uc;
    mac2::Cache* cache{};
};

struct MacContext {
    explicit MacContext(MacUnitRef mac1, MacUnitRef mac2, MacUnitRef mac3)
        : units{mac1, mac2, mac3} {}

    [[nodiscard]] MacUnitRef& selected() noexcept {
        return units[static_cast<std::size_t>(macAtivo)];
    }
    [[nodiscard]] const MacUnitRef& selected() const noexcept {
        return units[static_cast<std::size_t>(macAtivo)];
    }

    MacAtivo macAtivo{MacAtivo::Mac1};
    std::array<MacUnitRef, 3> units;
    std::string assemblyCode;
    std::string statusMessage;
    double clockIntervalMs{1000.0};
    int cacheSize{3};
    RegisterDisplayMode registerDisplayMode{RegisterDisplayMode::Hex};
    IPSMeter ipsMeter;
    std::string binaryBaseAddress{"0x00000000"};
    std::string binaryStatus;
    std::array<MemoryViewState, 3> memoryViews{
        MemoryViewState{true}, MemoryViewState{false}, MemoryViewState{false}};
    bool logOpen{};
    bool assemblyErrorOpen{};
    std::string assemblyError;
    std::string problematicInstruction;
    PerformanceTracker racePerformance;
    bool comparisonRunning{};
    double comparisonIntervalMs{250.0};
    std::size_t comparisonProgramSize{};
    std::string comparisonError;
    bool comparisonReady{};
    bool turboMode{};
    std::shared_ptr<const std::array<Estado, 3>> stateSnapshot;
};

} // namespace rv32i
