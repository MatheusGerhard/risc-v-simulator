#include <catch2/catch_test_macros.hpp>

#include "mac1/componentes/clock.hpp"
#include "mac1/componentes/memory.hpp"
#include "mac1/controlUnit.hpp"

#include <array>

using namespace rv32i;

namespace {
constexpr std::array<std::uint8_t, 20> program{
    0x93, 0x00, 0x40, 0x06, 0x13, 0x01, 0xa0, 0x0f, 0xb3, 0x81,
    0x20, 0x00, 0x23, 0x20, 0x30, 0x02, 0x03, 0x22, 0x00, 0x02};
}

TEST_CASE("Raw binary loader preserves little endian words and runs from arbitrary bases",
          "[binary-loader]") {
    SimulationLog log;
    Memory memory(128, log);
    REQUIRE(memory.loadRawBinary(program, 0) == program.size());
    CHECK(memory.readByte(0, false) == 0x93);
    CHECK(memory.readByte(3, false) == 0x06);
    CHECK(memory.readWordUnsigned(0) == 0x06400093U);

    ControlUnit cpu(memory, log);
    cpu.registradores().write(1, 999);
    cpu.registradores().write(2, 888);
    cpu.hardReset(0);
    CHECK(cpu.registradores().readUInt(1) == 0U);
    CHECK(cpu.registradores().readUInt(2) == 0U);
    CHECK(cpu.registradores().readPC() == 0U);
    CHECK(cpu.getEstado().stage == "FETCH");

    Clock clock;
    clock.pulso(cpu);
    CHECK(cpu.registradores().readInt(1) == 100);
    clock.pulso(cpu);
    CHECK(cpu.registradores().readInt(2) == 250);
    clock.pulso(cpu);
    CHECK(cpu.registradores().readInt(3) == 350);
    clock.pulso(cpu);
    CHECK(memory.readWord(32) == 350);
    clock.pulso(cpu);
    CHECK(cpu.registradores().readInt(4) == 350);

    memory.clearMemory();
    constexpr std::uint32_t base = 0x100;
    REQUIRE(memory.loadRawBinary(program, base) == program.size());
    CHECK(memory.readWordUnsigned(0) == 0x06400093U); // 0x100 wraps to zero in 128-byte RAM
    cpu.hardReset(base);
    CHECK(cpu.registradores().readPC() == base);
    clock.reset();
    clock.pulso(cpu);
    CHECK(cpu.registradores().readInt(1) == 100);
    CHECK(cpu.registradores().readPC() == base + 4);
}
