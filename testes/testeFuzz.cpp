#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include "mac1/componentes/memory.hpp"
#include "mac1/controlUnit.hpp"

#include <cstdint>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace rv32i;
using json = nlohmann::json;

namespace {
class XorShift32 {
public:
    explicit XorShift32(std::uint32_t state) : state_(state) {}
    std::uint32_t next() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return state_;
    }
private:
    std::uint32_t state_;
};

std::uint32_t encI(std::int32_t imm, unsigned rs1, unsigned f3, unsigned rd, unsigned opcode = 0x13) {
    return ((static_cast<std::uint32_t>(imm) & 0xfffU) << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | opcode;
}
std::uint32_t encR(unsigned f7, unsigned rs2, unsigned rs1, unsigned f3, unsigned rd) {
    return (f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | 0x33U;
}
std::uint32_t encS(std::int32_t imm, unsigned rs2, unsigned rs1, unsigned f3) {
    const auto bits = static_cast<std::uint32_t>(imm) & 0xfffU;
    return (((bits >> 5) & 0x7fU) << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) |
           ((bits & 31U) << 7) | 0x23U;
}
std::uint32_t encU(std::uint32_t imm, unsigned rd, unsigned opcode) {
    return ((imm & 0xfffffU) << 12) | (rd << 7) | opcode;
}

std::vector<std::uint32_t> generateWords() {
    XorShift32 random(0x5eed1234U);
    std::vector<std::uint32_t> words{encU(0xc3, 31, 0x37), encI(1280, 31, 0, 31)};
    constexpr unsigned rF3[] = {0, 1, 2, 3, 4, 5, 6, 7};
    constexpr unsigned loadF3[] = {0, 1, 2, 4, 5};
    constexpr unsigned storeF3[] = {0, 1, 2};
    for (int i = 0; i < 100000; ++i) {
        const auto kind = random.next() % 8;
        const auto rd = random.next() % 31;
        const auto rs1 = random.next() % 31;
        const auto rs2 = random.next() % 31;
        if (kind == 0) {
            auto imm = static_cast<std::int32_t>(random.next() & 0xfffU);
            if (imm & 0x800) imm -= 0x1000;
            words.push_back(encI(imm, rs1, 0, rd));
        } else if (kind == 1) {
            const auto f3 = rF3[random.next() % 8];
            const auto f7 = ((f3 == 0 || f3 == 5) && (random.next() & 1)) ? 0x20U : 0U;
            words.push_back(encR(f7, rs2, rs1, f3, rd));
        } else if (kind == 2) {
            const auto f3 = random.next() % 8;
            std::uint32_t imm;
            if (f3 == 1) imm = random.next() & 31U;
            else if (f3 == 5) imm = ((random.next() & 1U) ? 0x400U : 0U) | (random.next() & 31U);
            else {
                auto signedImm = static_cast<std::int32_t>(random.next() & 0xfffU);
                if (signedImm & 0x800) signedImm -= 0x1000;
                imm = static_cast<std::uint32_t>(signedImm);
            }
            words.push_back(encI(static_cast<std::int32_t>(imm), rs1, f3, rd));
        } else if (kind == 3) {
            const auto imm = random.next() & 0xffU;
            const auto f3 = loadF3[random.next() % 5];
            words.push_back(encI(static_cast<std::int32_t>(imm), 31, f3, rd, 0x03));
        } else if (kind == 4) {
            const auto imm = random.next() & 0xffU;
            const auto f3 = storeF3[random.next() % 3];
            words.push_back(encS(static_cast<std::int32_t>(imm), rs2, 31, f3));
        } else if (kind == 5) words.push_back(encU(random.next() & 0xfffffU, rd, 0x37));
        else if (kind == 6) words.push_back(encU(random.next() & 0xfffffU, rd, 0x17));
        else {
            const auto f3 = random.next() % 8;
            auto imm = random.next() & 0xfffU;
            if (f3 == 1) imm &= 31U;
            else if (f3 == 5) imm = ((random.next() & 1U) ? 0x400U : 0U) | (random.next() & 31U);
            else {
                auto signedImm = static_cast<std::int32_t>(imm);
                if (signedImm & 0x800) signedImm -= 0x1000;
                imm = static_cast<std::uint32_t>(signedImm);
            }
            words.push_back(encI(static_cast<std::int32_t>(imm), rs1, f3, rd));
        }
    }
    return words;
}

std::string stateHash(const ControlUnit& cpu, const Memory& memory, std::uint32_t dataBase) {
    std::uint32_t first = 0x811c9dc5U, second = 0x9e3779b9U;
    auto mix = [&](std::uint8_t byte) {
        first = (first ^ byte) * 0x01000193U;
        second = (second ^ byte) * 0x85ebca6bU;
        second ^= second >> 13;
    };
    auto mixWord = [&](std::uint32_t word) {
        mix(static_cast<std::uint8_t>(word)); mix(static_cast<std::uint8_t>(word >> 8));
        mix(static_cast<std::uint8_t>(word >> 16)); mix(static_cast<std::uint8_t>(word >> 24));
    };
    mixWord(cpu.registradores().readPC());
    for (const auto reg : cpu.registradores().readAll()) mixWord(reg);
    for (std::uint32_t address = dataBase; address < dataBase + 256; ++address)
        mix(static_cast<std::uint8_t>(memory.readByte(address, false)));
    std::ostringstream firstPart;
    firstPart << std::hex << std::setfill('0') << std::setw(8) << first;
    const auto signedSecond = std::bit_cast<std::int32_t>(second);
    std::ostringstream secondPart;
    secondPart << std::hex;
    if (signedSecond < 0) {
        const auto magnitude = static_cast<std::uint32_t>(-(static_cast<std::int64_t>(signedSecond)));
        std::ostringstream negative;
        negative << '-' << std::hex << magnitude;
        auto text = negative.str();
        if (text.size() < 8) text.insert(0, 8 - text.size(), '0');
        secondPart << text;
    } else {
        secondPart << std::setfill('0') << std::setw(8) << static_cast<std::uint32_t>(signedSecond);
    }
    return firstPart.str() + secondPart.str();
}
}

TEST_CASE("Fixed-seed 100k instruction differential fuzz matches JavaScript oracle", "[fuzz][parity]") {
    std::ifstream stream(std::filesystem::path(RV_GOLDEN_DIR) / "fuzz_seed_5eed1234.json");
    REQUIRE(stream.good());
    const auto oracle = json::parse(stream);
    const auto words = generateWords();
    const auto oracleWords = oracle.at("words").get<std::vector<std::uint32_t>>();
    REQUIRE(words.size() == oracleWords.size());
    for (std::size_t i = 0; i < words.size(); ++i) {
        INFO("first generated instruction mismatch at index " << i);
        REQUIRE(words[i] == oracleWords[i]);
    }
    REQUIRE(oracle.at("instructionCount").get<std::size_t>() == words.size());

    SimulationLog log;
    Memory memory(oracle.at("memorySize").get<std::size_t>(), log);
    for (std::size_t i = 0; i < words.size(); ++i)
        memory.writeWord(static_cast<std::uint32_t>(i * 4), words[i]);
    ControlUnit cpu(memory, log);
    cpu.hardReset(0);
    const auto hashes = oracle.at("stateHashes");
    for (std::size_t instruction = 0; instruction < words.size(); ++instruction) {
        for (std::uint8_t subcycle = 1; subcycle <= 5; ++subcycle)
            cpu.rodarCiclo(subcycle, instruction);
        REQUIRE(stateHash(cpu, memory, oracle.at("dataBase").get<std::uint32_t>()) ==
                hashes.at(instruction).get<std::string>());
    }
    CHECK(cpu.registradores().readPC() == oracle.at("finalPc").get<std::uint32_t>());
    const auto finalRegisters = cpu.registradores().readAll();
    CHECK(std::vector<std::uint32_t>(finalRegisters.begin(), finalRegisters.end()) ==
          oracle.at("finalRegisters").get<std::vector<std::uint32_t>>());
    std::vector<std::uint8_t> data;
    const auto base = oracle.at("dataBase").get<std::uint32_t>();
    for (std::uint32_t i = 0; i < 256; ++i) data.push_back(static_cast<std::uint8_t>(memory.readByte(base + i, false)));
    CHECK(data == oracle.at("finalData").get<std::vector<std::uint8_t>>());
}
