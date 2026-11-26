#pragma once

#include <cstdint>
#include <string_view>

namespace rv32i {

enum class AluOperation { Add, Sub, Sll, Slt, Sltu, Xor, Srl, Sra, Or, And, PassA, NotA };
[[nodiscard]] AluOperation parseAluOperation(std::string_view operation) noexcept;

class ArithmeticLogicUnit {
public:
    void write(std::int32_t a, std::int32_t b, AluOperation operation = AluOperation::Add) noexcept;
    void write(std::int32_t a, std::int32_t b, std::string_view operation) noexcept;
    [[nodiscard]] std::int32_t calcular() noexcept;
    [[nodiscard]] bool testBranchCondition(std::string_view operation,
                                           std::int32_t a, std::int32_t b) const noexcept;
    [[nodiscard]] std::int32_t result() const noexcept;
    [[nodiscard]] std::uint32_t resultUnsigned() const noexcept;
    [[nodiscard]] bool zero() const noexcept;
    [[nodiscard]] bool negative() const noexcept;

private:
    std::int32_t a_{};
    std::int32_t b_{};
    AluOperation operation_{AluOperation::Add};
    std::int32_t result_{};
    bool zero_{true};
    bool negative_{};
};

} // namespace rv32i
