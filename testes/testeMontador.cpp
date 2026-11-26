#include <catch2/catch_test_macros.hpp>

#include "mac1/componentes/memory.hpp"
#include "montador/macroinstrucoes.hpp"
#include "montador/montador.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace rv32i;

TEST_CASE("Immediate and memory operand parsing matches the JavaScript formats",
          "[montador][parsing]") {
    Montador assembler;
    REQUIRE(assembler.parseImmediate("-17") == -17.0);
    REQUIRE(assembler.parseImmediate("0x2A") == 42.0);
    REQUIRE(assembler.parseImmediate("-0X2A") == -42.0);
    REQUIRE(assembler.parseImmediate("0b1011") == 11.0);
    REQUIRE(assembler.parseImmediate("-0B101") == -5.0);
    REQUIRE(assembler.parseImmediate("0x10garbage") == 16.0);
    REQUIRE(std::isnan(assembler.parseImmediate("not-a-number")));

    const auto memory = assembler.parseMemoryOperand(" -8 (s0) ");
    REQUIRE(memory.has_value());
    REQUIRE(memory->offset == "-8");
    REQUIRE(memory->baseReg == "s0");
    REQUIRE_FALSE(assembler.parseMemoryOperand("8(s0)tail").has_value());
}

TEST_CASE("RISC-V format encoders match the JavaScript golden words", "[montador][encoding]") {
    REQUIRE(encodeRType(0, 3, 2, 0, 1, OPCODES::R_TYPE) == 0x003100b3U);
    REQUIRE(encodeIType(-1, 2, 0, 1, OPCODES::I_ALU) == 0xfff10093U);
    REQUIRE(encodeSType(-8, 3, 2, 2, OPCODES::S_TYPE) == 0xfe312c23U);
    REQUIRE(encodeBType(-16, 2, 1, 0, OPCODES::B_TYPE) == 0xfe2088e3U);
    REQUIRE(encodeUType(0xabcde000, 1, OPCODES::U_LUI) == 0xabcde0b7U);
    REQUIRE(encodeJType(-2048, 1, OPCODES::J_JAL) == 0x801ff0efU);
    REQUIRE(detecta_tipo_instrucao("SRAI", 1, 2, 0, 31) == 0x41f15093U);
}

TEST_CASE("Montador strips comments, handles labels case-insensitively, and resolves PC offsets",
          "[montador][labels]") {
    Montador assembler("Loop: addi x1, x0, -1 ; inline comment\n"
                       "  BNE x1, zero, lOoP # case-insensitive\n");
    REQUIRE_FALSE(assembler.main().has_value());
    REQUIRE(assembler.labelTable.at("LOOP") == 0U);
    REQUIRE(assembler.macroinstrucoes.size() == 2);
    REQUIRE(assembler.macroinstrucoes[0].byteAddress == 0U);
    REQUIRE(assembler.macroinstrucoes[1].byteAddress == 4U);
    REQUIRE(assembler.macroinstrucoes[1].imm == -4.0);
    REQUIRE(assembler.macroinstrucoes[1].labelRef == "LOOP");
    REQUIRE(assembler.microinstrucoes == std::vector<std::uint32_t>{0xfff00093U, 0xfe009ee3U});
}

TEST_CASE("All 22 documented pseudo-instructions expand with the JavaScript rules",
          "[montador][pseudoinstructions]") {
    Montador assembler(
        "nop\n"
        "mv x1, x2\n"
        "not x1, x2\n"
        "neg x1, x2\n"
        "seqz x1, x2\n"
        "snez x1, x2\n"
        "sltz x1, x2\n"
        "sgtz x1, x2\n"
        "beqz x1, done\n"
        "bnez x1, done\n"
        "blez x1, done\n"
        "bgez x1, done\n"
        "bltz x1, done\n"
        "bgtz x1, done\n"
        "bgt x1, x2, done\n"
        "ble x1, x2, done\n"
        "bgtu x1, x2, done\n"
        "bleu x1, x2, done\n"
        "j done\n"
        "jr x1\n"
        "ret\n"
        "li x3, 2048\n"
        "done: ebreak\n");
    (void)assembler.primeira_passagem();

    REQUIRE(assembler.macroinstrucoes.size() == 24);
    const std::vector<std::string> expected{
        "ADDI", "ADDI", "XORI", "SUB", "SLTIU", "SLTU", "SLT", "SLT", "BEQ", "BNE",
        "BGE", "BGE", "BLT", "BLT", "BLT", "BGE", "BLTU", "BGEU", "JAL", "JALR",
        "JALR", "LUI", "ADDI", "EBREAK"};
    for (std::size_t i = 0; i < expected.size(); ++i)
        REQUIRE(assembler.macroinstrucoes[i].opcode == expected[i]);
}

TEST_CASE("LI uses one or two instructions at signed 12-bit and 32-bit boundaries",
          "[montador][li]") {
    struct Golden {
        const char* immediate;
        std::vector<std::uint32_t> words;
    };
    const std::array<Golden, 10> cases{{
        {"-2049", {0xfffff0b7U, 0x7ff08093U}},
        {"-2048", {0x80000093U}},
        {"-1", {0xfff00093U}},
        {"0", {0x00000093U}},
        {"2047", {0x7ff00093U}},
        {"2048", {0x000010b7U, 0x80008093U}},
        {"4095", {0x000010b7U, 0xfff08093U}},
        {"2147483647", {0x800000b7U, 0xfff08093U}},
        {"2147483648", {0x800000b7U, 0x00008093U}},
        {"4294967295", {0x000000b7U, 0xfff08093U}},
    }};

    for (const auto& golden : cases) {
        Montador assembler(std::string("li x1, ") + golden.immediate);
        REQUIRE_FALSE(assembler.main().has_value());
        REQUIRE(assembler.microinstrucoes == golden.words);
    }

    Montador signedHex("li x1, -0x80000000");
    REQUIRE_FALSE(signedHex.main().has_value());
    REQUIRE(signedHex.microinstrucoes == std::vector<std::uint32_t>{0x800000b7U, 0x00008093U});
}

TEST_CASE("Montador preserves JavaScript error messages", "[montador][errors]") {
    const std::array<std::pair<std::string, std::string>, 7> cases{{
        {"add x1, x2", "Erro na instrução 'add x1, x2' (PC 0): ADD requer 3 registradores: rd, rs1, rs2"},
        {"addi x1, x2", "Erro na instrução 'addi x1, x2' (PC 0): ADDI requer: rd, rs1, imm"},
        {"sw x3, nope", "Erro na instrução 'sw x3, nope' (PC 0): Formato de memória inválido para SW: nope"},
        {"beq x1, x2", "Erro na instrução 'beq x1, x2' (PC 0): BEQ requer: rs1, rs2, label"},
        {"lui x1", "Erro na instrução 'lui x1' (PC 0): LUI requer: rd, imm"},
        {"foo x1", "Erro na instrução 'foo x1' (PC 0): Instrução não reconhecida: FOO"},
        {"jal", "Erro na instrução 'jal' (PC 0): Cannot read properties of undefined (reading 'toUpperCase')"},
    }};
    for (const auto& [source, expected] : cases) {
        Montador assembler(source);
        const auto error = assembler.main();
        REQUIRE(error.has_value());
        REQUIRE(*error == expected);
    }
}

TEST_CASE("Montador loads encoded words into injected memory and registers the log catalog",
          "[montador][memory]") {
    SimulationLog log;
    Memory memory(128, log);
    Montador assembler("start: addi x1, x0, 9\njal x0, start\n");
    REQUIRE_FALSE(assembler.main().has_value());
    assembler.preencherMemoria(memory, log);
    REQUIRE(memory.readWordUnsigned(0) == 0x00900093U);
    REQUIRE(memory.readWordUnsigned(4) == 0xffdff06fU);
}
