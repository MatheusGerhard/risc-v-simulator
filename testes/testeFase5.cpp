#include <catch2/catch_test_macros.hpp>

#include "src/components/Memory/BinaryUploader/BinaryUploader.hpp"
#include "src/hooks/useIPS.hpp"
#include "src/simulador.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

using namespace rv32i;

TEST_CASE("IPS formatting and one-second sampling match the React hook", "[fase5][ips]") {
    CHECK(formatIPS(0.0) == "0 IPS");
    CHECK(formatIPS(4.25) == "4.3 IPS");
    CHECK(formatIPS(10.4) == "10 IPS");
    CHECK(formatIPS(999.9) == "1000 IPS");
    CHECK(formatIPS(1'000.0) == "1.0k IPS");
    CHECK(formatIPS(1'234'567.0) == "1.2M IPS");
    CHECK(formatIPS(900.0, false) == "0 IPS");

    IPSMeter meter;
    const auto start = IPSMeter::TimePoint{} + std::chrono::seconds(4);
    meter.update(100, false, start);
    CHECK(meter.ips() == 0.0);
    meter.update(100, true, start);
    meter.update(125, true, start + std::chrono::milliseconds(500));
    CHECK(meter.ips() == 0.0);
    meter.update(150, true, start + std::chrono::seconds(1));
    CHECK(meter.ips() == 50.0);
    meter.update(150, false, start + std::chrono::seconds(2));
    CHECK(meter.ips() == 0.0);
}

TEST_CASE("Binary upload parses decimal/hex bases and follows the reset sequence", "[fase5][binary]") {
    REQUIRE(parseBaseAddress("0x00000020") == 0x20U);
    REQUIRE(parseBaseAddress("0X20") == 0x20U);
    REQUIRE(parseBaseAddress("32") == 32U);
    CHECK_FALSE(parseBaseAddress("0x").has_value());
    CHECK_FALSE(parseBaseAddress("-1").has_value());
    CHECK_FALSE(parseBaseAddress("4294967296").has_value());

    Simulador simulator;
    auto& context = simulator.context();
    auto& machine = context.selected();
    machine.memory.writeWord(0, 0xffffffffU);
    machine.uc.registradores().write(2, 88);
    machine.clock.setVelocidade(1.0);
    machine.clock.iniciar(machine.uc, Clock::TimePoint{} + std::chrono::seconds(1));

    const auto filename = "rv_phase5_upload_" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()) + ".bin";
    const auto path = std::filesystem::temp_directory_path() / filename;
    constexpr std::array<std::uint8_t, 4> program{0x93, 0x00, 0x50, 0x00}; // addi x1, x0, 5
    {
        std::ofstream file(path, std::ios::binary);
        REQUIRE(file.good());
        file.write(reinterpret_cast<const char*>(program.data()), static_cast<std::streamsize>(program.size()));
    }
    context.binaryBaseAddress = "0x20";
    REQUIRE(loadBinaryFile(context, path));
    std::filesystem::remove(path);

    CHECK(machine.memory.readWordUnsigned(0) == 0U);
    CHECK(machine.memory.readWordUnsigned(0x20) == 0x00500093U);
    CHECK(machine.uc.registradores().readPC() == 0x20U);
    CHECK(machine.uc.registradores().readUInt(2) == 0U);
    CHECK_FALSE(machine.clock.emExecucao);
    CHECK(machine.clock.getSC() == 1U);
    CHECK(machine.clock.getCycles() == 0U);
    CHECK(machine.uc.getEstado().stage == "FETCH");
}
