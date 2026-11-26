#pragma once

#include "../../src/services/simulationLog.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace rv32i {

class Memory {
public:
    explicit Memory(std::size_t sizeInBytes = 128,
                    SimulationLog& log = SimulationLog::instance());

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::int32_t readByte(std::uint32_t address, bool signedValue = true) const noexcept;
    [[nodiscard]] std::uint16_t readHalfUnsigned(std::uint32_t address) const noexcept;
    [[nodiscard]] std::int32_t readHalf(std::uint32_t address, bool signedValue = true) const noexcept;
    [[nodiscard]] std::uint32_t readWordUnsigned(std::uint32_t address) const noexcept;
    [[nodiscard]] std::int32_t readWord(std::uint32_t address) const noexcept;
    [[nodiscard]] std::string readWordBinary(std::uint32_t address) const;
    [[nodiscard]] std::string readWordHex(std::uint32_t address) const;
    [[nodiscard]] std::uint32_t read(std::uint32_t address) const noexcept;

    void writeByte(std::uint32_t address, std::uint8_t value);
    void writeHalf(std::uint32_t address, std::uint16_t value);
    void writeWord(std::uint32_t address, std::uint32_t value);
    void write(std::uint32_t address, std::uint32_t value);
    void writeMontador(std::uint32_t address, std::uint32_t value);
    std::size_t loadRawBinary(std::span<const std::uint8_t> bytes, std::uint32_t baseAddress = 0);
    void clearMemory();
    [[nodiscard]] std::vector<std::uint8_t> bytes() const;
    [[nodiscard]] std::vector<std::uint32_t> words() const;

private:
    [[nodiscard]] std::size_t normalize(std::uint32_t address) const noexcept;
    [[nodiscard]] std::uint32_t readWordAt(std::size_t address) const noexcept;
    std::vector<std::uint8_t> bytes_;
    SimulationLog& log_;
};

} // namespace rv32i
