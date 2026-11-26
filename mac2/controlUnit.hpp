#pragma once

#include "../mac1/controlUnit.hpp"
#include "componentes/cache.hpp"

namespace rv32i::mac2 {

class ControlUnit : public rv32i::ControlUnit {
public:
    explicit ControlUnit(Memory& memory, SimulationLog& log = SimulationLog::instance(),
                         std::size_t cacheSize = 3,
                         CacheMapping mapping = CacheMapping::Direct);
    void setCacheSize(std::size_t cacheSize);
    [[nodiscard]] Cache& cache() noexcept;
    [[nodiscard]] const Cache& cache() const noexcept;
    [[nodiscard]] std::uint64_t cacheHits() const noexcept override;
    [[nodiscard]] std::uint64_t cacheMisses() const noexcept override;
    [[nodiscard]] std::uint64_t cachePenaltyCycles() const noexcept override;

protected:
    [[nodiscard]] std::int32_t readMemoryByte(std::uint32_t address, bool signedValue) override;
    [[nodiscard]] std::int32_t readMemoryHalf(std::uint32_t address, bool signedValue) override;
    [[nodiscard]] std::int32_t readMemoryWord(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t readMemoryWordUnsigned(std::uint32_t address) override;
    void writeMemoryByte(std::uint32_t address, std::uint8_t value) override;
    void writeMemoryHalf(std::uint32_t address, std::uint16_t value) override;
    void writeMemoryWord(std::uint32_t address, std::uint32_t value) override;

private:
    Cache cache_;
};

} // namespace rv32i::mac2
