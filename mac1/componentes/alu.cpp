#include "alu.hpp"

#include <bit>
#include <cctype>
#include <string>

namespace rv32i {
namespace {
std::string normalize(std::string_view operation) {
    std::string value(operation);
    for (auto& ch : value) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    return value;
}
} // namespace

AluOperation parseAluOperation(std::string_view operation) noexcept {
    try {
        const auto op = normalize(operation);
        if (op == "SUB") return AluOperation::Sub;
        if (op == "SLL") return AluOperation::Sll;
        if (op == "SLT") return AluOperation::Slt;
        if (op == "SLTU") return AluOperation::Sltu;
        if (op == "XOR") return AluOperation::Xor;
        if (op == "SRL") return AluOperation::Srl;
        if (op == "SRA") return AluOperation::Sra;
        if (op == "OR") return AluOperation::Or;
        if (op == "AND" || op == "01") return AluOperation::And;
        if (op == "PASS_A" || op == "10") return AluOperation::PassA;
        if (op == "NOT_A" || op == "11") return AluOperation::NotA;
    } catch (...) {
        return AluOperation::Add;
    }
    return AluOperation::Add;
}

void ArithmeticLogicUnit::write(std::int32_t a, std::int32_t b,
                                AluOperation operation) noexcept {
    a_ = a;
    b_ = b;
    operation_ = operation;
}

void ArithmeticLogicUnit::write(std::int32_t a, std::int32_t b,
                                std::string_view operation) noexcept {
    a_ = a;
    b_ = b;
    operation_ = parseAluOperation(operation);
}

std::int32_t ArithmeticLogicUnit::calcular() noexcept {
    const auto aBits = std::bit_cast<std::uint32_t>(a_);
    const auto bBits = std::bit_cast<std::uint32_t>(b_);
    const auto shift = bBits & 0x1fU;
    std::uint32_t bits = 0;
    switch (operation_) {
    case AluOperation::Add: bits = aBits + bBits; break;
    case AluOperation::Sub: bits = aBits - bBits; break;
    case AluOperation::Sll: bits = aBits << shift; break;
    case AluOperation::Slt: bits = a_ < b_ ? 1U : 0U; break;
    case AluOperation::Sltu: bits = aBits < bBits ? 1U : 0U; break;
    case AluOperation::Xor: bits = aBits ^ bBits; break;
    case AluOperation::Srl: bits = aBits >> shift; break;
    case AluOperation::Sra: bits = std::bit_cast<std::uint32_t>(a_ >> shift); break;
    case AluOperation::Or: bits = aBits | bBits; break;
    case AluOperation::And: bits = aBits & bBits; break;
    case AluOperation::PassA: bits = aBits; break;
    case AluOperation::NotA: bits = ~aBits; break;
    }
    result_ = std::bit_cast<std::int32_t>(bits);
    zero_ = bits == 0;
    negative_ = (bits & 0x80000000U) != 0;
    return result_;
}

bool ArithmeticLogicUnit::testBranchCondition(std::string_view operation, std::int32_t a,
                                              std::int32_t b) const noexcept {
    try {
        const auto op = normalize(operation);
        if (op == "BEQ") return a == b;
        if (op == "BNE") return a != b;
        if (op == "BLT") return a < b;
        if (op == "BGE") return a >= b;
        if (op == "BLTU") return std::bit_cast<std::uint32_t>(a) < std::bit_cast<std::uint32_t>(b);
        if (op == "BGEU") return std::bit_cast<std::uint32_t>(a) >= std::bit_cast<std::uint32_t>(b);
    } catch (...) {
        return false;
    }
    return false;
}

std::int32_t ArithmeticLogicUnit::result() const noexcept { return result_; }
std::uint32_t ArithmeticLogicUnit::resultUnsigned() const noexcept {
    return std::bit_cast<std::uint32_t>(result_);
}
bool ArithmeticLogicUnit::zero() const noexcept { return zero_; }
bool ArithmeticLogicUnit::negative() const noexcept { return negative_; }

} // namespace rv32i
