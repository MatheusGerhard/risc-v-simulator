#pragma once

#include "../../mac1/componentes/memory.hpp"
#include "../../src/services/performanceTracker.hpp"
#include "../../src/services/simulationLog.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace rv32i::mac2 {

enum class CacheMapping { Direct, AssociativeFifo };

struct CacheLineSnapshot {
    std::size_t index{};
    bool valid{};
    bool hasContent{};
    std::uint32_t address{};
    std::array<std::uint32_t, 4> words{};
};

class Cache {
public:
    static constexpr std::uint32_t blockBytes = 16;
    static constexpr std::uint64_t missPenaltyCycles = 4;

    Cache(Memory& ram, SimulationLog& log, Processor processor, std::size_t size = 3,
          CacheMapping mapping = CacheMapping::Direct);
    void resize(std::size_t size);
    void reset();
    [[nodiscard]] std::uint32_t read(std::uint32_t address, std::size_t width);
    void write(std::uint32_t address, std::size_t width, std::uint32_t value);
    [[nodiscard]] std::vector<CacheLineSnapshot> getSnapshot() const;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::uint64_t hits() const noexcept;
    [[nodiscard]] std::uint64_t misses() const noexcept;
    [[nodiscard]] std::uint64_t penaltyCycles() const noexcept;
    [[nodiscard]] CacheMapping mapping() const noexcept;

private:
    struct Line {
        bool valid{};
        bool hasTag{};
        std::uint32_t blockAddress{};
        std::array<std::uint32_t, 4> words{};
    };

    [[nodiscard]] std::size_t findLine(std::uint32_t blockAddress) const noexcept;
    [[nodiscard]] std::size_t chooseLine(std::uint32_t blockAddress) noexcept;
    [[nodiscard]] std::size_t accessLine(std::uint32_t blockAddress, bool write);
    [[nodiscard]] std::uint8_t readCachedByte(std::uint32_t address) const noexcept;

    Memory& ram_;
    SimulationLog& log_;
    Processor processor_;
    CacheMapping mapping_;
    std::vector<Line> lines_;
    std::size_t fifoNext_{};
    std::uint64_t hits_{};
    std::uint64_t misses_{};
    std::uint64_t penaltyCycles_{};
};

} // namespace rv32i::mac2
