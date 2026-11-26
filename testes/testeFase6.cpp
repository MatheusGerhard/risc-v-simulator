#include "../src/services/performanceTracker.hpp"
#include "../src/simulador.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <string>

using namespace rv32i;

TEST_CASE("PerformanceTracker names follow MacAtivo and retain 120 recent instructions", "[fase6]") {
    REQUIRE(std::string(PerformanceTracker::processorName(MacAtivo::Mac1)) == "RV32I");
    REQUIRE(std::string(PerformanceTracker::processorName(MacAtivo::Mac2)) == "RV32I (Cache D)");
    REQUIRE(std::string(PerformanceTracker::processorName(MacAtivo::Mac3)) == "RV32I (Cache A)");

    PerformanceTracker tracker;
    for (std::uint64_t i = 0; i < 125; ++i) {
        tracker.instructionStarted(MacAtivo::Mac2, static_cast<std::uint32_t>(i * 4),
                                  "ADDI x1, x0, 1", i);
        tracker.instructionFinished(MacAtivo::Mac2, i + 1);
    }
    tracker.updateRuntime(MacAtivo::Mac2, 125, 8, 2);
    const auto& state = tracker.state(MacAtivo::Mac2);
    REQUIRE(state.completedInstructions == 125);
    REQUIRE(state.totalCycles == 125);
    REQUIRE(state.ipc == Catch::Approx(1.0));
    REQUIRE(state.cacheHits == 8);
    REQUIRE(state.cacheMisses == 2);
    REQUIRE(state.cacheHitRate == Catch::Approx(0.8));
    REQUIRE(state.instructions.size() == 120);
    REQUIRE(state.instructions.front().address == 20);
}

TEST_CASE("Comparison runs five subcycles on three isolated program memories", "[fase6]") {
    Simulador simulator;
    auto& context = simulator.context();
    context.assemblyCode = "addi x1, x0, 42\nsw x1, 0(x0)\n";
    REQUIRE(simulator.prepararComparacao());
    REQUIRE(context.units[0].memory.bytes() == context.units[1].memory.bytes());
    REQUIRE(context.units[1].memory.bytes() == context.units[2].memory.bytes());

    for (int instruction = 0; instruction < 2; ++instruction)
        simulator.avancarComparacaoInstrucao();

    for (const auto& lane : context.units) {
        REQUIRE(lane.clock.getSC() == 1);
        REQUIRE(lane.clock.getCycles() == 2);
        REQUIRE(lane.uc.getCompletedInstructions() == 2);
        REQUIRE(lane.memory.readWordUnsigned(0) == 42);
    }
    context.units[0].memory.writeWord(64, 0xAABBCCDDU);
    REQUIRE(context.units[1].memory.readWordUnsigned(64) == 0);
    REQUIRE(context.units[2].memory.readWordUnsigned(64) == 0);
    for (const auto& state : context.racePerformance.snapshot()) {
        REQUIRE(state.completedInstructions == 2);
        REQUIRE(state.instructions.size() == 2);
    }
}
