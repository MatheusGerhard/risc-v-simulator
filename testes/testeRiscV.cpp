#include <catch2/catch_test_macros.hpp>

#include "mac1/componentes/alu.hpp"
#include "mac1/componentes/clock.hpp"
#include "mac1/componentes/memory.hpp"
#include "mac1/componentes/registers.hpp"
#include "mac1/controlUnit.hpp"
#include "montador/montador.hpp"

#include <array>
#include <cstdint>

using namespace rv32i;

TEST_CASE("Port of testeRiscV register checks", "[registers]") {
    Registers registers;
    registers.write(0, 100);
    REQUIRE(registers.readInt(0) == 0);
    registers.write(1, 42);
    REQUIRE(registers.readInt(1) == 42);
    registers.write("sp", 0x1000);
    REQUIRE(registers.readInt(2) == 0x1000);
    registers.writePC(0x40);
    REQUIRE(registers.readPC() == 0x40);
}

TEST_CASE("Port of testeRiscV little-endian RAM checks", "[memory]") {
    SimulationLog log;
    Memory memory(512, log);
    memory.writeWord(0x100, 0x12345678U);
    REQUIRE(memory.readByte(0x100, false) == 0x78);
    REQUIRE(memory.readByte(0x101, false) == 0x56);
    REQUIRE(memory.readByte(0x102, false) == 0x34);
    REQUIRE(memory.readByte(0x103, false) == 0x12);
    REQUIRE(memory.readHalf(0x100, false) == 0x5678);
    REQUIRE(memory.readWordUnsigned(0x100) == 0x12345678U);
}

TEST_CASE("Port of testeRiscV ALU checks", "[alu]") {
    ArithmeticLogicUnit alu;
    alu.write(10, 20, "ADD");
    REQUIRE(alu.calcular() == 30);
    alu.write(10, 20, "SUB");
    REQUIRE(alu.calcular() == -10);
    alu.write(-5, 0, "SLT");
    REQUIRE(alu.calcular() == 1);
    alu.write(-5, 0, "SLTU");
    REQUIRE(alu.calcular() == 0);
    alu.write(1, 4, "SLL");
    REQUIRE(alu.calcular() == 16);
    alu.write(16, 2, "SRL");
    REQUIRE(alu.calcular() == 4);
    alu.write(-16, 2, "SRA");
    REQUIRE(alu.calcular() == -4);
}

TEST_CASE("Port of testeRiscV executes arithmetic, memory, and BEQ", "[control-unit]") {
    SimulationLog log;
    Memory memory(128, log);
    Montador assembler("addi x1, x0, 15\n"
                       "addi x2, x0, 25\n"
                       "add x3, x1, x2\n"
                       "sw x3, 0(x0)\n"
                       "lw x4, 0(x0)\n"
                       "beq x3, x4, sucesso\n"
                       "addi x5, x0, 99\n"
                       "sucesso:\n"
                       "addi x5, x0, 100\n");
    REQUIRE_FALSE(assembler.main().has_value());
    REQUIRE(assembler.microinstrucoes == std::vector<std::uint32_t>{
        0x00f00093U, 0x01900113U, 0x002081b3U, 0x00302023U,
        0x00002203U, 0x00418463U, 0x06300293U, 0x06400293U});
    assembler.preencherMemoria(memory, log);

    ControlUnit cpu(memory, log);
    cpu.hardReset();
    Clock clock;
    for (int i = 0; i < 7; ++i) clock.pulso(cpu);

    REQUIRE(cpu.registradores().readInt(1) == 15);
    REQUIRE(cpu.registradores().readInt(2) == 25);
    REQUIRE(cpu.registradores().readInt(3) == 40);
    REQUIRE(memory.readWord(0) == 40);
    REQUIRE(cpu.registradores().readInt(4) == 40);
    REQUIRE(cpu.registradores().readInt(5) == 100);
}

TEST_CASE("Legacy ALU operation codes remain supported", "[alu]") {
    ArithmeticLogicUnit alu;
    alu.write(0x00f0, 0x0ff0, "00");
    REQUIRE(alu.calcular() == 0x10e0);
    alu.write(0x00f0, 0x0ff0, "01");
    REQUIRE(alu.calcular() == 0x00f0);
    alu.write(0x00f0, 0, "10");
    REQUIRE(alu.calcular() == 0x00f0);
    alu.write(0x00f0, 0, "11");
    REQUIRE(alu.calcular() == ~0x00f0);
}
