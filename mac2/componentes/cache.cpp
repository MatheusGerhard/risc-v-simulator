#include "cache.hpp"

#include <algorithm>

namespace rv32i::mac2 {
namespace {
}

Cache::Cache(Memory& ram, SimulationLog& log, Processor processor, std::size_t size,
             CacheMapping mapping)
    : ram_(ram), log_(log), processor_(processor), mapping_(mapping),
      lines_(std::max<std::size_t>(size, 1)) {}

void Cache::resize(std::size_t size) {
    lines_.assign(std::max<std::size_t>(size, 1), Line{});
    fifoNext_ = 0;
    hits_ = misses_ = penaltyCycles_ = 0;
}

void Cache::reset() {
    for (auto& line : lines_) line = {};
    fifoNext_ = 0;
    hits_ = misses_ = penaltyCycles_ = 0;
}

std::size_t Cache::findLine(std::uint32_t blockAddress) const noexcept {
    for (std::size_t index = 0; index < lines_.size(); ++index) {
        if (lines_[index].valid && lines_[index].blockAddress == blockAddress) return index;
    }
    return lines_.size();
}

std::size_t Cache::chooseLine(std::uint32_t blockAddress) noexcept {
    if (mapping_ == CacheMapping::Direct)
        return static_cast<std::size_t>((blockAddress / blockBytes) % lines_.size());
    const auto selected = fifoNext_;
    fifoNext_ = (fifoNext_ + 1) % lines_.size();
    return selected;
}

std::size_t Cache::accessLine(std::uint32_t blockAddress, bool writeAccess) {
    auto index = findLine(blockAddress);
    if (index != lines_.size()) {
        ++hits_;
        log_.logCacheHit(processor_, blockAddress);
        return index;
    }

    ++misses_;
    penaltyCycles_ += missPenaltyCycles;
    log_.logCacheMiss(processor_, blockAddress);
    index = chooseLine(blockAddress);
    auto& line = lines_[index];
    line = {};
    line.blockAddress = blockAddress;
    line.hasTag = true;
    line.valid = !writeAccess;
    if (!writeAccess) {
        for (std::size_t word = 0; word < line.words.size(); ++word)
            line.words[word] = ram_.readWordUnsigned(blockAddress + static_cast<std::uint32_t>(word * 4));
    }
    return index;
}

std::uint8_t Cache::readCachedByte(std::uint32_t address) const noexcept {
    const auto blockAddress = address & ~(blockBytes - 1U);
    const auto index = findLine(blockAddress);
    if (index == lines_.size()) return 0;
    const auto offset = address - blockAddress;
    const auto word = lines_[index].words[offset / 4U];
    return static_cast<std::uint8_t>((word >> ((offset % 4U) * 8U)) & 0xffU);
}

std::uint32_t Cache::read(std::uint32_t address, std::size_t width) {
    width = std::clamp<std::size_t>(width, 1, 4);
    std::uint32_t value = 0;
    for (std::size_t byte = 0; byte < width; ++byte) {
        const auto current = address + static_cast<std::uint32_t>(byte);
        const auto block = current & ~(blockBytes - 1U);
        if (byte == 0 || block != ((address + static_cast<std::uint32_t>(byte - 1)) & ~(blockBytes - 1U)))
            (void)accessLine(block, false);
        value |= static_cast<std::uint32_t>(readCachedByte(current)) << (byte * 8U);
    }
    return value;
}

void Cache::write(std::uint32_t address, std::size_t width, std::uint32_t value) {
    width = std::clamp<std::size_t>(width, 1, 4);
    std::array<std::uint32_t, 2> touchedBlocks{};
    std::array<std::size_t, 2> touchedLines{};
    std::size_t touchedCount = 0;
    for (std::size_t byte = 0; byte < width; ++byte) {
        const auto current = address + static_cast<std::uint32_t>(byte);
        const auto block = current & ~(blockBytes - 1U);
        auto touched = std::find(touchedBlocks.begin(), touchedBlocks.begin() +
                                 static_cast<std::ptrdiff_t>(touchedCount), block);
        std::size_t lineIndex;
        if (touched == touchedBlocks.begin() + static_cast<std::ptrdiff_t>(touchedCount)) {
            lineIndex = accessLine(block, true);
            touchedBlocks[touchedCount] = block;
            touchedLines[touchedCount] = lineIndex;
            ++touchedCount;
        } else {
            lineIndex = touchedLines[static_cast<std::size_t>(touched - touchedBlocks.begin())];
        }
        auto& line = lines_[lineIndex];
        const auto offset = current - block;
        const auto shift = static_cast<unsigned int>((offset % 4U) * 8U);
        auto& word = line.words[offset / 4U];
        word = (word & ~(0xffU << shift)) |
               (((value >> (byte * 8U)) & 0xffU) << shift);
    }
    if (width == 1) ram_.writeByte(address, static_cast<std::uint8_t>(value));
    else if (width == 2) ram_.writeHalf(address, static_cast<std::uint16_t>(value));
    else ram_.writeWord(address, value);
}

std::vector<CacheLineSnapshot> Cache::getSnapshot() const {
    std::vector<CacheLineSnapshot> snapshot;
    snapshot.reserve(lines_.size());
    for (std::size_t index = 0; index < lines_.size(); ++index) {
        const auto& line = lines_[index];
        snapshot.push_back({index, line.valid, line.hasTag, line.blockAddress, line.words});
    }
    return snapshot;
}

std::size_t Cache::size() const noexcept { return lines_.size(); }
std::uint64_t Cache::hits() const noexcept { return hits_; }
std::uint64_t Cache::misses() const noexcept { return misses_; }
std::uint64_t Cache::penaltyCycles() const noexcept { return penaltyCycles_; }
CacheMapping Cache::mapping() const noexcept { return mapping_; }

} // namespace rv32i::mac2
