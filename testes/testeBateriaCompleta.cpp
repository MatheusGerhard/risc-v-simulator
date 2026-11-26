#include <catch2/catch_test_macros.hpp>

#include "mac1/componentes/clock.hpp"
#include "mac1/componentes/memory.hpp"
#include "mac1/controlUnit.hpp"
#include "montador/montador.hpp"

using namespace rv32i;

TEST_CASE("RV32I battery assembles and runs LUI/AUIPC, ALU, memory, branches and jumps",
          "[control-unit][battery][montador]") {
    constexpr auto assembly = R"(
        lui  x1, 0x12345
        auipc x2, 0x1000
        addi x3, x0, 10
        addi x4, x3, -4
        sub  x5, x3, x4
        xor  x6, x3, x4
        or   x7, x3, x4
        and  x8, x3, x4
        slli x9, x3, 2
        srli x10, x9, 1
        srai x11, x4, 1
        slti x12, x4, 10
        sltu x13, x4, x3
        addi x14, x0, 0x100
        sw   x3, 0(x14)
        lw   x15, 0(x14)
        sh   x3, 4(x14)
        lhu  x16, 4(x14)
        sb   x3, 6(x14)
        lbu  x17, 6(x14)
        beq  x3, x15, branch_ok
        addi x18, x0, 1
    branch_ok:
        addi x18, x0, 2
        jal  x1, subrotina
        addi x19, x0, 100
        j fim
    subrotina:
        addi x20, x0, 77
        ret
    fim:
        addi x21, x0, 999
    )";

    SimulationLog log;
    Memory memory(512, log);
    Montador assembler(assembly);
    REQUIRE_FALSE(assembler.main().has_value());
    REQUIRE(assembler.microinstrucoes.size() == 29);
    assembler.preencherMemoria(memory, log);

    ControlUnit cpu(memory, log);
    cpu.hardReset();
    Clock clock;
    for (int instruction = 0; instruction < 50 && cpu.registradores().readInt(21) != 999;
         ++instruction)
        clock.pulso(cpu);

    const auto& regs = cpu.registradores();
    REQUIRE(regs.readUInt(1) == 96U);
    REQUIRE(regs.readUInt(2) == 0x01000004U);
    REQUIRE(regs.readInt(3) == 10);
    REQUIRE(regs.readInt(4) == 6);
    REQUIRE(regs.readInt(5) == 4);
    REQUIRE(regs.readInt(6) == (10 ^ 6));
    REQUIRE(regs.readInt(7) == (10 | 6));
    REQUIRE(regs.readInt(8) == (10 & 6));
    REQUIRE(regs.readInt(9) == 40);
    REQUIRE(regs.readInt(10) == 20);
    REQUIRE(regs.readInt(11) == 3);
    REQUIRE(regs.readInt(12) == 1);
    REQUIRE(regs.readInt(13) == 1);
    REQUIRE(regs.readInt(15) == 10);
    REQUIRE(regs.readInt(16) == 10);
    REQUIRE(regs.readInt(17) == 10);
    REQUIRE(regs.readInt(18) == 2);
    REQUIRE(regs.readInt(19) == 100);
    REQUIRE(regs.readInt(20) == 77);
    REQUIRE(regs.readInt(21) == 999);
    REQUIRE(cpu.getCompletedInstructions() == 28);
}
