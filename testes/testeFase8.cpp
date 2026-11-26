#include "../src/simulador.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <thread>

using namespace rv32i;

TEST_CASE("Turbo worker publishes immutable state snapshots while logs reset concurrently", "[fase8][concurrency]") {
    Simulador simulator;
    auto& context = simulator.context();
    context.assemblyCode = "addi x1, x1, 1\njal x0, 0\n";
    REQUIRE(simulator.prepararComparacao());

    simulator.setTurboMode(true);
    REQUIRE(simulator.turboMode());
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(150);
    std::uint64_t observedInstructions = 0;
    std::size_t observedEvents = 0;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto snapshot = simulator.stateSnapshot();
        REQUIRE(snapshot != nullptr);
        observedInstructions = std::max(observedInstructions, (*snapshot)[0].instrucoesConcluidas);
        observedEvents = std::max(observedEvents, SimulationLog::instance().getEvents().size());
        (void)context.racePerformance.snapshot();
        SimulationLog::instance().clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    simulator.setTurboMode(false);

    REQUIRE_FALSE(simulator.turboMode());
    const auto finalSnapshot = simulator.stateSnapshot();
    REQUIRE(finalSnapshot != nullptr);
    REQUIRE(std::max(observedInstructions, (*finalSnapshot)[0].instrucoesConcluidas) > 0);
    REQUIRE(observedEvents > 0);
}
