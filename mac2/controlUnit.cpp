#include "controlUnit.hpp"

#include <bit>

namespace rv32i::mac2 {

ControlUnit::ControlUnit(Memory& memory, SimulationLog& log, std::size_t cacheSize,
                         CacheMapping mapping)
    : rv32i::ControlUnit(memory, log),
      cache_(memory, log, mapping == CacheMapping::Direct ? Processor::Mac2 : Processor::Mac3,
             cacheSize, mapping) {}

void ControlUnit::setCacheSize(std::size_t cacheSize) { cache_.resize(cacheSize); }
Cache& ControlUnit::cache() noexcept { return cache_; }
const Cache& ControlUnit::cache() const noexcept { return cache_; }
std::uint64_t ControlUnit::cacheHits() const noexcept { return cache_.hits(); }
std::uint64_t ControlUnit::cacheMisses() const noexcept { return cache_.misses(); }
std::uint64_t ControlUnit::cachePenaltyCycles() const noexcept { return cache_.penaltyCycles(); }

std::int32_t ControlUnit::readMemoryByte(std::uint32_t address, bool signedValue) {
    const auto raw = static_cast<std::uint8_t>(cache_.read(address, 1));
    return signedValue ? static_cast<std::int32_t>(std::bit_cast<std::int8_t>(raw)) : raw;
}
std::int32_t ControlUnit::readMemoryHalf(std::uint32_t address, bool signedValue) {
    const auto raw = static_cast<std::uint16_t>(cache_.read(address, 2));
    return signedValue ? static_cast<std::int32_t>(std::bit_cast<std::int16_t>(raw)) : raw;
}
std::int32_t ControlUnit::readMemoryWord(std::uint32_t address) {
    return std::bit_cast<std::int32_t>(cache_.read(address, 4));
}
std::uint32_t ControlUnit::readMemoryWordUnsigned(std::uint32_t address) {
    return cache_.read(address, 4);
}
void ControlUnit::writeMemoryByte(std::uint32_t address, std::uint8_t value) {
    cache_.write(address, 1, value);
}
void ControlUnit::writeMemoryHalf(std::uint32_t address, std::uint16_t value) {
    cache_.write(address, 2, value);
}
void ControlUnit::writeMemoryWord(std::uint32_t address, std::uint32_t value) {
    cache_.write(address, 4, value);
}

} // namespace rv32i::mac2
