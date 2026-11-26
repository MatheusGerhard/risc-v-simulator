#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "mac1/componentes/memory.hpp"
#include "mac1/controlUnit.hpp"
#include "montador/montador.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace rv32i;
using json = nlohmann::json;

namespace {
std::vector<std::uint8_t> bytesFrom(const json& input) {
    return input.get<std::vector<std::uint8_t>>();
}
}

TEST_CASE("Golden JavaScript traces match C++ at every subcycle", "[parity][golden]") {
    const std::filesystem::path directory{RV_GOLDEN_DIR};
    std::size_t cases = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() != ".json") continue;
        std::ifstream stream(entry.path());
        const auto golden = json::parse(stream);
        if (golden.at("kind") != "trace") continue;
        ++cases;

        SimulationLog log;
        Memory memory(golden.at("memorySize").get<std::size_t>(), log);
        std::vector<std::uint32_t> words;
        if (!golden.at("assembly").is_null()) {
            Montador assembler(golden.at("assembly").get<std::string>());
            REQUIRE_FALSE(assembler.main().has_value());
            words = assembler.microinstrucoes;
            const auto base = golden.at("baseAddress").get<std::uint32_t>();
            for (std::size_t i = 0; i < words.size(); ++i)
                memory.writeWord(base + static_cast<std::uint32_t>(i * 4), words[i]);
        } else {
            const auto raw = bytesFrom(golden.at("binaryBytes"));
            memory.loadRawBinary(raw, golden.at("baseAddress").get<std::uint32_t>());
            for (std::size_t offset = 0; offset < raw.size(); offset += 4)
                words.push_back(memory.readWordUnsigned(static_cast<std::uint32_t>(
                    golden.at("baseAddress").get<std::uint32_t>() + offset)));
        }
        REQUIRE(words == golden.at("programWords").get<std::vector<std::uint32_t>>());

        ControlUnit cpu(memory, log);
        const auto base = golden.at("baseAddress").get<std::uint32_t>();
        cpu.hardReset(base);
        std::uint8_t subcycle = 1;
        std::uint64_t cycles = 0;
        for (const auto& expected : golden.at("trace")) {
            cpu.rodarCiclo(subcycle, cycles);
            const auto observed = cpu.getEstado(subcycle, cycles);
            CHECK(subcycle == expected.at("subciclo").get<std::uint8_t>());
            CHECK(cycles == expected.at("ciclos").get<std::uint64_t>());
            CHECK(observed.stage == expected.at("stage").get<std::string>());
            CHECK(observed.pc == expected.at("pc").get<std::uint32_t>());
            CHECK(std::vector<std::uint32_t>(observed.registers.begin(), observed.registers.end()) ==
                  expected.at("regs").get<std::vector<std::uint32_t>>());
            CHECK(observed.mar == expected.at("mar").get<std::uint32_t>());
            CHECK(static_cast<std::uint32_t>(observed.aluRes) == expected.at("aluOut").get<std::uint32_t>());
            CHECK(observed.branchTaken == expected.at("branchTaken").get<bool>());
            CHECK(observed.jumpTaken == expected.at("jumpTaken").get<bool>());
            if (++subcycle > 5) { subcycle = 1; ++cycles; }
        }
        CHECK(memory.bytes() == bytesFrom(golden.at("finalMemory")));
    }
    REQUIRE(cases >= 6);
}

TEST_CASE("Assembler words and diagnostics retain the JavaScript contract", "[parity][assembler]") {
    struct Example { std::string source; std::vector<std::uint32_t> words; };
    const std::vector<Example> words{
        {"add x1, x2, x3", {0x003100b3U}},
        {"addi x1, x2, -1", {0xfff10093U}},
        {"sw x3, -8(x2)", {0xfe312c23U}},
        {"beq x1, x2, -16", {0xfe2088e3U}},
        {"lui x1, 0xabcde", {0xabcde0b7U}},
        {"jal x1, -2048", {0x801ff0efU}},
    };
    for (const auto& item : words) {
        Montador assembler(item.source);
        REQUIRE_FALSE(assembler.main().has_value());
        REQUIRE(assembler.microinstrucoes == item.words);
    }
    const std::vector<std::pair<std::string, std::string>> errors{
        {"add x1, x2", "Erro na instrução 'add x1, x2' (PC 0): ADD requer 3 registradores: rd, rs1, rs2"},
        {"addi x1, x2", "Erro na instrução 'addi x1, x2' (PC 0): ADDI requer: rd, rs1, imm"},
        {"sw x3, nope", "Erro na instrução 'sw x3, nope' (PC 0): Formato de memória inválido para SW: nope"},
        {"beq x1, x2", "Erro na instrução 'beq x1, x2' (PC 0): BEQ requer: rs1, rs2, label"},
        {"lui x1", "Erro na instrução 'lui x1' (PC 0): LUI requer: rd, imm"},
        {"foo x1", "Erro na instrução 'foo x1' (PC 0): Instrução não reconhecida: FOO"},
        {"jal", "Erro na instrução 'jal' (PC 0): Cannot read properties of undefined (reading 'toUpperCase')"},
    };
    for (const auto& [source, expected] : errors) {
        Montador assembler(source);
        const auto error = assembler.main();
        REQUIRE(error.has_value());
        CHECK(*error == expected);
    }
}

TEST_CASE("LI matches one/two word JavaScript expansions at 12 and 32 bit limits", "[parity][li]") {
    const std::vector<std::pair<std::string, std::vector<std::uint32_t>>> cases{
        {"-2049", {0xfffff0b7U, 0x7ff08093U}}, {"-2048", {0x80000093U}},
        {"2047", {0x7ff00093U}}, {"2048", {0x000010b7U, 0x80008093U}},
        {"2147483647", {0x800000b7U, 0xfff08093U}},
        {"2147483648", {0x800000b7U, 0x00008093U}},
        {"4294967295", {0x000000b7U, 0xfff08093U}},
    };
    for (const auto& [imm, expected] : cases) {
        Montador assembler("li x1, " + imm);
        REQUIRE_FALSE(assembler.main().has_value());
        CHECK(assembler.microinstrucoes == expected);
    }
}
