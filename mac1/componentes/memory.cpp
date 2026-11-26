#include "memory.hpp"

#include "registers.hpp"

#include <algorithm>
#include <bit>
#include <stdexcept>

namespace rv32i {

Memory::Memory(std::size_t sizeInBytes, SimulationLog& log) : bytes_(sizeInBytes), log_(log) {
    if (sizeInBytes == 0 || (sizeInBytes & (sizeInBytes - 1)) != 0)
        throw std::invalid_argument("RAM size must be a non-zero power of two");
}

std::size_t Memory::size() const noexcept { return bytes_.size(); }

std::size_t Memory::normalize(std::uint32_t address) const noexcept {
    return static_cast<std::size_t>(address) & (bytes_.size() - 1);
}

std::int32_t Memory::readByte(std::uint32_t address, bool signedValue) const noexcept {
    const auto value = bytes_[normalize(address)];
    if (signedValue) return std::bit_cast<std::int8_t>(value);
    return static_cast<std::int32_t>(value);
}

std::uint16_t Memory::readHalfUnsigned(std::uint32_t address) const noexcept {
    const auto base = normalize(address);
    const auto next = (base + 1) & (bytes_.size() - 1);
    return static_cast<std::uint16_t>(bytes_[base]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes_[next]) << 8U);
}

std::int32_t Memory::readHalf(std::uint32_t address, bool signedValue) const noexcept {
    const auto value = readHalfUnsigned(address);
    if (signedValue) return std::bit_cast<std::int16_t>(value);
    return static_cast<std::int32_t>(value);
}

std::uint32_t Memory::readWordAt(std::size_t base) const noexcept {
    const auto mask = bytes_.size() - 1;
    std::uint32_t value = 0;
    for (unsigned int i = 0; i < 4; ++i)
        value |= static_cast<std::uint32_t>(bytes_[(base + i) & mask]) << (i * 8U);
    return value;
}

std::uint32_t Memory::readWordUnsigned(std::uint32_t address) const noexcept {
    return readWordAt(normalize(address));
}

std::int32_t Memory::readWord(std::uint32_t address) const noexcept {
    return std::bit_cast<std::int32_t>(readWordUnsigned(address));
}

std::string Memory::readWordBinary(std::uint32_t address) const {
    return toBinary32(readWordUnsigned(address));
}

std::string Memory::readWordHex(std::uint32_t address) const {
    return toHex32(readWordUnsigned(address));
}

std::uint32_t Memory::read(std::uint32_t address) const noexcept {
    return readWordUnsigned(address);
}

void Memory::writeByte(std::uint32_t address, std::uint8_t value) {
    const auto addr = normalize(address);
    const auto wordAddr = addr & ~std::size_t{3};
    const auto previous = toBinary32(readWordAt(wordAddr));
    bytes_[addr] = value;
    log_.logMemoryWrite(static_cast<std::uint32_t>(addr), previous,
                        toBinary32(readWordAt(wordAddr)));
}

void Memory::writeHalf(std::uint32_t address, std::uint16_t value) {
    const auto addr = normalize(address);
    const auto wordAddr = addr & ~std::size_t{3};
    const auto previous = toBinary32(readWordAt(wordAddr));
    const auto mask = bytes_.size() - 1;
    bytes_[addr] = static_cast<std::uint8_t>(value & 0xffU);
    bytes_[(addr + 1) & mask] = static_cast<std::uint8_t>((value >> 8U) & 0xffU);
    log_.logMemoryWrite(static_cast<std::uint32_t>(addr), previous,
                        toBinary32(readWordAt(wordAddr)));
}

void Memory::writeWord(std::uint32_t address, std::uint32_t value) {
    const auto addr = normalize(address);
    const auto previous = toBinary32(readWordAt(addr));
    const auto mask = bytes_.size() - 1;
    for (unsigned int i = 0; i < 4; ++i)
        bytes_[(addr + i) & mask] = static_cast<std::uint8_t>((value >> (i * 8U)) & 0xffU);
    log_.logMemoryWrite(static_cast<std::uint32_t>(addr), previous,
                        toBinary32(readWordAt(addr)));
}

void Memory::write(std::uint32_t address, std::uint32_t value) { writeWord(address, value); }
void Memory::writeMontador(std::uint32_t address, std::uint32_t value) { writeWord(address, value); }

std::size_t Memory::loadRawBinary(std::span<const std::uint8_t> bytes,
                                  std::uint32_t baseAddress) {
    const auto start = normalize(baseAddress);
    const auto mask = bytes_.size() - 1;
    for (std::size_t i = 0; i < bytes.size(); ++i) bytes_[(start + i) & mask] = bytes[i];
    log_.clearMacroInstructionCatalog();
    return bytes.size();
}

void Memory::clearMemory() {
    std::fill(bytes_.begin(), bytes_.end(), std::uint8_t{0});
    log_.clearMacroInstructionCatalog();
}
std::vector<std::uint8_t> Memory::bytes() const { return bytes_; }

std::vector<std::uint32_t> Memory::words() const {
    std::vector<std::uint32_t> result;
    result.reserve(bytes_.size() / 4);
    for (std::size_t address = 0; address + 3 < bytes_.size(); address += 4)
        result.push_back(readWordAt(address));
    return result;
}

} // namespace rv32i
