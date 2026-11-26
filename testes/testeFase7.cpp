#include "../mac2/componentes/cache.hpp"
#include "../mac3/controlUnit.hpp"
#include "../src/simulador.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>

using namespace rv32i;

TEST_CASE("Direct cache records hits and misses and writes through to RAM", "[fase7][cache]") {
    SimulationLog log;
    Memory ram(256, log);
    ram.writeWord(0, 0x11223344U);
    ram.writeWord(16, 0xAABBCCDDU);
    log.clear();
    mac2::Cache cache(ram, log, MacAtivo::Mac2, 1, mac2::CacheMapping::Direct);

    REQUIRE(cache.read(0, 4) == 0x11223344U);
    REQUIRE(cache.read(4, 4) == 0x00000000U);
    cache.write(0, 4, 0xDEADBEEFU);
    REQUIRE(ram.readWordUnsigned(0) == 0xDEADBEEFU);
    REQUIRE(cache.read(0, 4) == 0xDEADBEEFU);
    cache.write(16, 4, 0x12345678U); // write miss: write-through, no write-allocate
    REQUIRE(ram.readWordUnsigned(16) == 0x12345678U);
    REQUIRE(cache.read(16, 4) == 0x12345678U);

    REQUIRE(cache.hits() == 3);
    REQUIRE(cache.misses() == 3);
    REQUIRE(cache.penaltyCycles() == 3 * mac2::Cache::missPenaltyCycles);
    const auto events = log.getEvents();
    REQUIRE(std::count_if(events.begin(), events.end(), [](const LogEvent& event) {
        return event.type == LogType::CacheHit;
    }) == 3);
    REQUIRE(std::count_if(events.begin(), events.end(), [](const LogEvent& event) {
        return event.type == LogType::CacheMiss;
    }) == 3);
}

TEST_CASE("Associative cache replaces lines in FIFO order and direct cache maps by index", "[fase7][cache]") {
    SimulationLog log;
    Memory ram(256, log);
    ram.writeWord(0, 10);
    ram.writeWord(16, 20);
    ram.writeWord(32, 30);

    mac2::Cache associative(ram, log, MacAtivo::Mac3, 2,
                            mac2::CacheMapping::AssociativeFifo);
    REQUIRE(associative.read(0, 4) == 10);
    REQUIRE(associative.read(16, 4) == 20);
    REQUIRE(associative.read(32, 4) == 30);
    auto snapshot = associative.getSnapshot();
    REQUIRE(snapshot[0].address == 32);
    REQUIRE(snapshot[1].address == 16);
    REQUIRE(associative.read(16, 4) == 20);
    REQUIRE(associative.read(0, 4) == 10);
    REQUIRE(associative.hits() == 1);
    REQUIRE(associative.misses() == 4);

    mac2::Cache writeThrough(ram, log, MacAtivo::Mac3, 2,
                             mac2::CacheMapping::AssociativeFifo);
    writeThrough.write(64, 4, 0x01020304U);
    REQUIRE(writeThrough.read(64, 4) == 0x01020304U);
    writeThrough.write(64, 4, 0xA0B0C0D0U);
    REQUIRE(writeThrough.read(64, 4) == 0xA0B0C0D0U);

    mac2::Cache direct(ram, log, MacAtivo::Mac2, 2, mac2::CacheMapping::Direct);
    REQUIRE(direct.read(0, 4) == 10);
    REQUIRE(direct.read(32, 4) == 30);
    REQUIRE(direct.getSnapshot()[0].address == 32);
    REQUIRE(direct.read(0, 4) == 10);
    REQUIRE(direct.misses() == 3);
}

TEST_CASE("Comparison reports distinct cache behavior and penalties while Mac1 stays direct", "[fase7][comparison]") {
    Simulador simulator;
    auto& context = simulator.context();
    context.cacheSize = 2;
    context.assemblyCode =
        "addi x1, x0, 7\n"
        "addi x2, x0, 11\n"
        "sw x1, 0(x0)\n"
        "sw x2, 32(x0)\n"
        "lw x3, 0(x0)\n"
        "lw x4, 32(x0)\n"
        "lw x5, 0(x0)\n";
    REQUIRE(simulator.prepararComparacao());
    for (int index = 0; index < 7; ++index) simulator.avancarComparacaoInstrucao();

    const auto& direct = context.racePerformance.state(MacAtivo::Mac2);
    const auto& associative = context.racePerformance.state(MacAtivo::Mac3);
    const auto& uncached = context.racePerformance.state(MacAtivo::Mac1);
    REQUIRE(context.units[0].uc.registradores().readInt(5) == 7);
    REQUIRE(context.units[1].uc.registradores().readInt(5) == 7);
    REQUIRE(context.units[2].uc.registradores().readInt(5) == 7);
    REQUIRE(uncached.cacheHits == 0);
    REQUIRE(uncached.cacheMisses == 0);
    REQUIRE(direct.cacheMisses == 5);
    REQUIRE(associative.cacheHits == 1);
    REQUIRE(associative.cacheMisses == 4);
    REQUIRE(direct.cyclePenaltyCycles > associative.cyclePenaltyCycles);
    REQUIRE(direct.totalCycles > associative.totalCycles);
    REQUIRE(associative.ipc > direct.ipc);
}
