#include "simulationLog.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

namespace rv32i {
namespace {
std::string hexAddress(std::uint32_t address) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << address;
    return out.str();
}
} // namespace

SimulationLog& SimulationLog::instance() {
    static SimulationLog log;
    return log;
}

std::uint64_t SimulationLog::subscribe(Subscriber callback) {
    std::uint64_t id;
    std::vector<LogEvent> snapshot;
    {
        std::lock_guard lock(mutex_);
        id = nextSubscription_++;
        subscribers_.emplace(id, callback);
        snapshot = events_;
    }
    if (callback) callback(snapshot);
    return id;
}

void SimulationLog::unsubscribe(std::uint64_t subscription) {
    std::lock_guard lock(mutex_);
    subscribers_.erase(subscription);
}

std::vector<LogEvent> SimulationLog::getEvents() const {
    std::lock_guard lock(mutex_);
    return events_;
}

void SimulationLog::clear() {
    std::vector<Subscriber> callbacks;
    {
        std::lock_guard lock(mutex_);
        events_.clear();
        nextId_ = 1;
        performance_.reset();
        for (const auto& [id, callback] : subscribers_) {
            (void)id;
            callbacks.push_back(callback);
        }
    }
    for (const auto& callback : callbacks) if (callback) callback({});
}

void SimulationLog::clearMacroInstructionCatalog() {
    std::lock_guard lock(mutex_);
    instructionCatalog_.clear();
}

void SimulationLog::registerMacroInstructions(
    std::vector<std::pair<std::uint32_t, std::string>> instructions) {
    std::lock_guard lock(mutex_);
    instructionCatalog_.clear();
    for (auto& [address, instruction] : instructions)
        instructionCatalog_.insert_or_assign(address, std::move(instruction));
}

void SimulationLog::addEvent(LogEvent event) {
    std::vector<Subscriber> callbacks;
    std::vector<LogEvent> snapshot;
    {
        std::lock_guard lock(mutex_);
        event.id = nextId_++;
        events_.insert(events_.begin(), std::move(event));
        if (events_.size() > maxEvents) events_.resize(maxEvents);
        snapshot = events_;
        for (const auto& [id, callback] : subscribers_) {
            (void)id;
            callbacks.push_back(callback);
        }
    }
    for (const auto& callback : callbacks) if (callback) callback(snapshot);
}

void SimulationLog::logMemoryWrite(std::uint32_t address, std::string previousValue,
                                   std::string value) {
    const auto addressText = hexAddress(address);
    addEvent({0, LogType::MemoryWrite, "[MEMORY WRITE]",
              "RAM[" + addressText + "]: " + previousValue + " -> " + value,
              address, std::move(previousValue), std::move(value)});
}

void SimulationLog::logInstruction(std::uint32_t address, std::string mnemonic,
                                   std::uint64_t cycle) {
    std::string instruction = std::move(mnemonic);
    {
        std::lock_guard lock(mutex_);
        if (const auto found = instructionCatalog_.find(address); found != instructionCatalog_.end())
            instruction = found->second;
    }
    std::ostringstream message;
    message << "Executando: " << instruction;
    if (instruction.empty()) message << " instrução no endereço " << hexAddress(address);
    (void)cycle;
    addEvent({0, LogType::Instruction, "[INSTRUCTION]", message.str(), address, {}, {}});
    performance_.instructionStarted(Processor::RV32I, cycle);
}

void SimulationLog::logCacheHit(Processor processor, std::uint32_t address) {
    addEvent({0, LogType::CacheHit, "[CACHE HIT]",
              std::string(PerformanceTracker::processorName(processor)) + ": Hit no endereço " + hexAddress(address),
              address, {}, {}});
}

void SimulationLog::logCacheMiss(Processor processor, std::uint32_t address) {
    addEvent({0, LogType::CacheMiss, "[CACHE MISS]",
              std::string(PerformanceTracker::processorName(processor)) + ": Miss no endereço " + hexAddress(address),
              address, {}, {}});
}

} // namespace rv32i
