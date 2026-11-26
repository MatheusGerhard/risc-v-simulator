#pragma once

#include "../mac2/controlUnit.hpp"

namespace rv32i::mac3 {

class ControlUnit final : public mac2::ControlUnit {
public:
    explicit ControlUnit(Memory& memory, SimulationLog& log = SimulationLog::instance(),
                         std::size_t cacheSize = 3);
    void setCacheSize(std::size_t cacheSize);
};

} // namespace rv32i::mac3
