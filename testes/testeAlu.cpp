#include <catch2/catch_test_macros.hpp>

#include "mac1/componentes/alu.hpp"

using namespace rv32i;

TEST_CASE("Port of testeAlu.js legacy operations and flags", "[alu]") {
    ArithmeticLogicUnit alu;

    alu.write(0, 0, "00");
    REQUIRE(alu.calcular() == 0);
    REQUIRE(alu.zero());
    REQUIRE_FALSE(alu.negative());

    alu.write(1, 1, "00");
    REQUIRE(alu.calcular() == 2);
    REQUIRE_FALSE(alu.zero());
    alu.write(7, 1, "00");
    REQUIRE(alu.calcular() == 8);

    alu.write(0b1010101010101010, 0b1111111100000000, "01");
    REQUIRE(alu.calcular() == 0b1010101000000000);

    alu.write(0b1111000011110000, 0, "10");
    REQUIRE(alu.calcular() == 0b1111000011110000);

    alu.write(0b0000111100001111, 0, "11");
    REQUIRE(alu.calcular() == ~0b0000111100001111);
    REQUIRE(alu.negative());
}
