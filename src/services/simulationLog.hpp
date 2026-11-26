#pragma once

#include "performanceTracker.hpp"

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rv32i {

enum class LogType { MemoryWrite, CacheHit, CacheMiss, Instruction };

struct LogEvent {
    std::uint64_t id{};
    LogType type{};
    std::string label;
    std::string message;
    std::uint32_t address{};
    std::string previousValue;
    std::string value;
};

class SimulationLog {
public:
    using Subscriber = std::function<void(const std::vector<LogEvent>&)>;
    static constexpr std::size_t maxEvents = 500;

    static SimulationLog& instance();
    std::uint64_t subscribe(Subscriber callback);
    void unsubscribe(std::uint64_t subscription);
    [[nodiscard]] std::vector<LogEvent> getEvents() const;
    void clear();
    void clearMacroInstructionCatalog();
    void registerMacroInstructions(std::vector<std::pair<std::uint32_t, std::string>> instructions);
    void logMemoryWrite(std::uint32_t address, std::string previousValue, std::string value);
    void logInstruction(std::uint32_t address, std::string mnemonic, std::uint64_t cycle);
    void logCacheHit(Processor processor, std::uint32_t address);
    void logCacheMiss(Processor processor, std::uint32_t address);

private:
    void addEvent(LogEvent event);
    mutable std::mutex mutex_;
    std::vector<LogEvent> events_;
    std::unordered_map<std::uint64_t, Subscriber> subscribers_;
    std::unordered_map<std::uint32_t, std::string> instructionCatalog_;
    std::uint64_t nextId_{1};
    std::uint64_t nextSubscription_{1};
    PerformanceTracker performance_;
};

} // namespace rv32i
