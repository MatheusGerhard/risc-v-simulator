#include "controlUnit.hpp"

namespace rv32i::mac3 {

ControlUnit::ControlUnit(Memory& memory, SimulationLog& log, std::size_t cacheSize)
    : mac2::ControlUnit(memory, log, cacheSize, mac2::CacheMapping::AssociativeFifo) {}

void ControlUnit::setCacheSize(std::size_t cacheSize) {
    mac2::ControlUnit::setCacheSize(cacheSize);
}

} // namespace rv32i::mac3
