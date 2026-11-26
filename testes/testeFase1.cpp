#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "mac1/componentes/alu.hpp"
#include "mac1/componentes/clock.hpp"
#include "mac1/componentes/memory.hpp"
#include "mac1/componentes/registers.hpp"
#include "mac1/controlUnit.hpp"
#include "src/services/performanceTracker.hpp"
#include "src/services/simulationLog.hpp"

#include <bit>
#include <cstdint>

using namespace rv32i;

TEST_CASE("Register file keeps x0 fixed and resolves ABI aliases", "[registers]") {
    Registers registers;
    registers.write(0, 99);
    registers.write("fp", -1);

    REQUIRE(registers.readInt(0) == 0);
    REQUIRE(registers.readUInt("s0") == 0xffffffffU);
    REQUIRE(registers.readHex(8) == "0xFFFFFFFF");
    REQUIRE(parseRegisterName(" x31,") == 31);
    REQUIRE_FALSE(parseRegisterName("x32").has_value());
    REQUIRE(toBinary32(1).substr(31) == "1");
}

TEST_CASE("RAM is little-endian, sign extends, and wraps at its size", "[memory]") {
    SimulationLog log;
    Memory memory(128, log);
    memory.writeWord(4, 0x80ff7f01U);
    REQUIRE(memory.readByte(4) == 1);
    REQUIRE(memory.readByte(7) == -128);
    REQUIRE(memory.readHalf(6) == -32513);
    REQUIRE(memory.readWordUnsigned(4) == 0x80ff7f01U);

    memory.writeWord(127, 0x12345678U);
    REQUIRE(memory.readWordUnsigned(127) == 0x12345678U);
    REQUIRE(memory.bytes()[127] == 0x78);
    REQUIRE(memory.bytes()[0] == 0x56);
}

TEST_CASE("ALU wraps 32-bit addition and handles signed and unsigned operations", "[alu]") {
    ArithmeticLogicUnit alu;
    alu.write(INT32_MAX, 1, AluOperation::Add);
    REQUIRE(alu.calcular() == INT32_MIN);
    REQUIRE(alu.negative());
    REQUIRE_FALSE(alu.zero());

    alu.write(-1, 1, "SLTU");
    REQUIRE(alu.calcular() == 0);
    REQUIRE(alu.testBranchCondition("BLTU", 1, -1));
    alu.write(-8, 1, "SRA");
    REQUIRE(alu.calcular() == -4);
}

TEST_CASE("Five subcycles execute one RV32I instruction", "[control-unit]") {
    SimulationLog log;
    Memory memory(128, log);
    memory.writeWord(0, 0x00500093U); // ADDI x1, x0, 5
    ControlUnit cpu(memory, log);
    cpu.hardReset();
    Clock clock;
    clock.pulso(cpu);

    REQUIRE(clock.getCycles() == 1);
    REQUIRE(clock.getSC() == 1);
    REQUIRE(cpu.registradores().readInt(1) == 5);
    REQUIRE(cpu.registradores().readPC() == 4);
    REQUIRE(cpu.getCompletedInstructions() == 1);
}

TEST_CASE("Frame-driven clock accumulates subcycles at the configured interval", "[clock]") {
    SimulationLog log;
    Memory memory(128, log);
    ControlUnit cpu(memory, log);
    cpu.hardReset();

    Clock clock;
    clock.setVelocidade(10);
    const auto start = Clock::TimePoint{} + std::chrono::seconds(1);
    clock.iniciar(cpu, start);
    REQUIRE(clock.atualizar(cpu, start + std::chrono::milliseconds(9)) == 0);
    REQUIRE(clock.atualizar(cpu, start + std::chrono::milliseconds(35)) == 3);
    REQUIRE(clock.getSC() == 4);
    REQUIRE(clock.atualizar(cpu, start + std::chrono::milliseconds(55)) == 2);
    REQUIRE(clock.getSC() == 1);
    REQUIRE(clock.getCycles() == 1);

    clock.pausar();
    REQUIRE(clock.atualizar(cpu, start + std::chrono::seconds(2)) == 0);
    clock.setVelocidade(0);
    REQUIRE(clock.intervaloMs == 1000.0);
}

TEST_CASE("Frame-driven clock caps catch-up work and drops excessive backlog", "[clock][safety]") {
    SimulationLog log;
    Memory memory(128, log);
    ControlUnit cpu(memory, log);
    cpu.hardReset();
    Clock clock;
    clock.setVelocidade(1.0);
    const auto start = Clock::TimePoint{} + std::chrono::seconds(2);
    clock.iniciar(cpu, start);

    REQUIRE(clock.atualizar(cpu, start + std::chrono::seconds(2)) == 100);
    REQUIRE(clock.atualizar(cpu, start + std::chrono::seconds(2) + std::chrono::milliseconds(1)) == 1);
}

TEST_CASE("Simulation log keeps the newest 500 events and notifies subscribers", "[services]") {
    SimulationLog log;
    std::size_t notifications = 0;
    const auto subscription = log.subscribe([&notifications](const std::vector<LogEvent>&) {
        ++notifications;
    });
    for (std::uint32_t i = 0; i < 501; ++i)
        log.logMemoryWrite(i, "before", "after");

    const auto events = log.getEvents();
    REQUIRE(events.size() == SimulationLog::maxEvents);
    REQUIRE(events.front().address == 500);
    REQUIRE(events.back().address == 1);
    REQUIRE(notifications == 502);
    log.unsubscribe(subscription);
}

TEST_CASE("Performance tracker updates cycle and IPC metrics", "[services]") {
    PerformanceTracker tracker;
    tracker.instructionStarted(Processor::RV32I, 0);
    tracker.instructionStarted(Processor::RV32I, 5);

    const auto& state = tracker.state(Processor::RV32I);
    REQUIRE(state.completedInstructions == 1);
    REQUIRE(state.totalCycles == 5);
    REQUIRE(state.ipc == Catch::Approx(0.2));
    REQUIRE(std::string(PerformanceTracker::processorName(Processor::RV32I_C1)) == "RV32I (Cache D)");
}
