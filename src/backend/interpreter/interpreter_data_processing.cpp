/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include <algorithm>

#include "backend/interpreter/interpreter.h"
#include "common/bit_util.h"
#include "common/safe_ops.h"
#include "common/u128.h"

namespace Dynarmic::Backend::Interpreter {

u64 Interpreter::ExecutePack2x32To1x64(u32 lo, u32 hi) {
    return (static_cast<u64>(hi) << 32) | lo;
}

Vector Interpreter::ExecutePack2x64To1x128(u64 lo, u64 hi) {
    return {lo, hi};
}

u32 Interpreter::ExecuteLeastSignificantWord(u64 a) {
    return static_cast<u32>(a);
}

ResultAndCarry<u32> Interpreter::ExecuteMostSignificantWord(u64 a) {
    return {static_cast<u32>(a >> 32), Common::Bit<31>(a)};
}

u16 Interpreter::ExecuteLeastSignificantHalf(u32 a) {
    return static_cast<u16>(a);
}

u8 Interpreter::ExecuteLeastSignificantByte(u32 a) {
    return static_cast<u8>(a);
}

bool Interpreter::ExecuteMostSignificantBit(u32 a) {
    return Common::MostSignificantBit(a);
}

bool Interpreter::ExecuteIsZero32(u32 a) {
    return a == 0;
}

bool Interpreter::ExecuteIsZero64(u64 a) {
    return a == 0;
}

bool Interpreter::ExecuteTestBit(u64 a, u8 bit_position) {
    return Common::Bit(bit_position, a);
}

u32 Interpreter::ExecuteConditionalSelect32(IR::Cond cond, u32 then_, u32 else_) {
    return ConditionPassed(cond) ? then_ : else_;
}

u64 Interpreter::ExecuteConditionalSelect64(IR::Cond cond, u64 then_, u64 else_) {
    return ConditionPassed(cond) ? then_ : else_;
}

NZCV Interpreter::ExecuteConditionalSelectNZCV(IR::Cond cond, NZCV then_, NZCV else_) {
    return ConditionPassed(cond) ? then_ : else_;
}

ResultAndCarry<u32> Interpreter::ExecuteLogicalShiftLeft32(u32 value, u8 shift, bool carry_in) {
    if (shift == 0) {
        return {value, carry_in};
    }

    const u32 result = Safe::LogicalShiftLeft(value, shift);
    const bool carry = Common::Bit<0>(Safe::LogicalShiftLeft(value, static_cast<int>(shift) + 32));
    return {result, carry};
}

u64 Interpreter::ExecuteLogicalShiftLeft64(u64 value, u8 shift) {
    return Safe::LogicalShiftLeft(value, shift);
}

ResultAndCarry<u32> Interpreter::ExecuteLogicalShiftRight32(u32 value, u8 shift, bool carry_in) {
    if (shift == 0) {
        return {value, carry_in};
    }

    const u32 result = Safe::LogicalShiftRight(value, shift);
    const bool carry = Common::Bit<0>(Safe::LogicalShiftRight(value, static_cast<int>(shift) - 1));
    return {result, carry};
}

u64 Interpreter::ExecuteLogicalShiftRight64(u64 value, u8 shift) {
    return Safe::LogicalShiftRight(value, shift);
}

ResultAndCarry<u32> Interpreter::ExecuteArithmeticShiftRight32(u32 value, u8 shift, bool carry_in) {
    if (shift == 0) {
        return {value, carry_in};
    }

    const u32 result = Safe::ArithmeticShiftRight(value, shift);
    const bool carry = Common::Bit<0>(Safe::ArithmeticShiftRight(value, static_cast<int>(shift) - 1));
    return {result, carry};
}

u64 Interpreter::ExecuteArithmeticShiftRight64(u64 value, u8 shift) {
    return Safe::ArithmeticShiftRight(value, shift);
}

ResultAndCarry<u32> Interpreter::ExecuteRotateRight32(u32 value, u8 shift, bool carry_in) {
    if (shift == 0) {
        return {value, carry_in};
    }

    const u32 result = Common::RotateRight(value, shift);
    const bool carry = Common::MostSignificantBit(result);
    return {result, carry};
}

u64 Interpreter::ExecuteRotateRight64(u64 value, u8 shift) {
    return Common::RotateRight(value, shift);
}

ResultAndCarry<u32> Interpreter::ExecuteRotateRightExtended(u32 value, bool carry_in) {
    return {Common::ModifyBit<31>(value >> 1, carry_in), Common::Bit<0>(value)};
}

ResultAndNZCV<u32> Interpreter::ExecuteAdd32(u32 a, u32 b, bool carry_in) {
    const u32 result = a + b + (carry_in ? 1 : 0);
    const bool negative = Common::MostSignificantBit(result);
    const bool zero = result == 0;
    const bool carry = result < a;
    const bool overflow = Common::MostSignificantBit((a ^ result) & ~(a ^ b));
    return {result, negative, zero, carry, overflow};
}

ResultAndNZCV<u64> Interpreter::ExecuteAdd64(u64 a, u64 b, bool carry_in) {
    const u64 result = a + b + (carry_in ? 1 : 0);
    const bool negative = Common::MostSignificantBit(result);
    const bool zero = result == 0;
    const bool carry = result < a;
    const bool overflow = Common::MostSignificantBit((a ^ result) & ~(a ^ b));
    return {result, negative, zero, carry, overflow};
}

ResultAndNZCV<u32> Interpreter::ExecuteSub32(u32 a, u32 b, bool carry_in) {
    const u32 result = a + ~b + (carry_in ? 1 : 0);
    const bool negative = Common::MostSignificantBit(result);
    const bool zero = result == 0;
    const bool carry = result < a;
    const bool overflow = Common::MostSignificantBit((a ^ result) & ~(a ^ ~b));
    return {result, negative, zero, carry, overflow};
}

ResultAndNZCV<u64> Interpreter::ExecuteSub64(u64 a, u64 b, bool carry_in) {
    const u64 result = a + ~b + (carry_in ? 1 : 0);
    const bool negative = Common::MostSignificantBit(result);
    const bool zero = result == 0;
    const bool carry = result < a;
    const bool overflow = Common::MostSignificantBit((a ^ result) & ~(a ^ ~b));
    return {result, negative, zero, carry, overflow};
}

u32 Interpreter::ExecuteMul32(u32 a, u32 b) {
    return a * b;
}

u64 Interpreter::ExecuteMul64(u64 a, u64 b) {
    return a * b;
}

u64 Interpreter::ExecuteSignedMultiplyHigh64(u64 a, u64 b) {
    const u64 unsigned_high = Multiply64To128(a, b).upper;
    const u64 correction = Common::MostSignificantBit(a & b);
    return unsigned_high + correction;
}

u64 Interpreter::ExecuteUnsignedMultiplyHigh64(u64 a, u64 b) {
    return Multiply64To128(a, b).upper;
}

u32 Interpreter::ExecuteUnsignedDiv32(u32 a, u32 b) {
    return a / b;
}

u64 Interpreter::ExecuteUnsignedDiv64(u64 a, u64 b) {
    return a / b;
}

s32 Interpreter::ExecuteSignedDiv32(s32 a, s32 b) {
    return a / b;
}

s64 Interpreter::ExecuteSignedDiv64(s64 a, s64 b) {
    return a / b;
}

u32 Interpreter::ExecuteAnd32(u32 a, u32 b) {
    return a & b;
}

u64 Interpreter::ExecuteAnd64(u64 a, u64 b) {
    return a & b;
}

u32 Interpreter::ExecuteEor32(u32 a, u32 b) {
    return a ^ b;
}

u64 Interpreter::ExecuteEor64(u64 a, u64 b) {
    return a ^ b;
}

u32 Interpreter::ExecuteOr32(u32 a, u32 b) {
    return a | b;
}

u64 Interpreter::ExecuteOr64(u64 a, u64 b) {
    return a | b;
}

u32 Interpreter::ExecuteNot32(u32 a) {
    return ~a;
}

u64 Interpreter::ExecuteNot64(u64 a) {
    return ~a;
}

s32 Interpreter::ExecuteSignExtendByteToWord(s8 a) {
    return static_cast<s32>(a);
}

s32 Interpreter::ExecuteSignExtendHalfToWord(s16 a) {
    return static_cast<s32>(a);
}

s64 Interpreter::ExecuteSignExtendByteToLong(s8 a) {
    return static_cast<s64>(a);
}

s64 Interpreter::ExecuteSignExtendHalfToLong(s16 a) {
    return static_cast<s64>(a);
}

s64 Interpreter::ExecuteSignExtendWordToLong(s32 a) {
    return static_cast<s64>(a);
}

u32 Interpreter::ExecuteZeroExtendByteToWord(u8 a) {
    return static_cast<u32>(a);
}

u32 Interpreter::ExecuteZeroExtendHalfToWord(u16 a) {
    return static_cast<u32>(a);
}

u64 Interpreter::ExecuteZeroExtendByteToLong(u8 a) {
    return static_cast<u64>(a);
}

u64 Interpreter::ExecuteZeroExtendHalfToLong(u16 a) {
    return static_cast<u64>(a);
}

u64 Interpreter::ExecuteZeroExtendWordToLong(u32 a) {
    return static_cast<u64>(a);
}

Vector Interpreter::ExecuteZeroExtendLongToQuad(u64 a) {
    return {a, 0};
}

u32 Interpreter::ExecuteByteReverseWord(u32 a) {
    return Common::Swap32(a);
}

u16 Interpreter::ExecuteByteReverseHalf(u16 a) {
    return Common::Swap16(a);
}

u64 Interpreter::ExecuteByteReverseDual(u64 a) {
    return Common::Swap64(a);
}

u32 Interpreter::ExecuteCountLeadingZeros32(u32 a) {
    return Common::CountLeadingZeros(a);
}

u64 Interpreter::ExecuteCountLeadingZeros64(u64 a) {
    return Common::CountLeadingZeros(a);
}

u32 Interpreter::ExecuteExtractRegister32(u32 lo, u32 hi, u8 shift) {
    const u32 result_lo = Safe::LogicalShiftRight(lo, shift);
    const u32 result_hi = Safe::LogicalShiftLeft(hi, 32 - static_cast<int>(shift));
    return result_hi | result_lo;
}

u64 Interpreter::ExecuteExtractRegister64(u64 lo, u64 hi, u8 shift) {
    const u64 result_lo = Safe::LogicalShiftRight(lo, shift);
    const u64 result_hi = Safe::LogicalShiftLeft(hi, 64 - static_cast<int>(shift));
    return result_hi | result_lo;
}

s32 Interpreter::ExecuteMaxSigned32(s32 a, s32 b) {
    return std::max(a, b);
}

s64 Interpreter::ExecuteMaxSigned64(s64 a, s64 b) {
    return std::max(a, b);
}

u32 Interpreter::ExecuteMaxUnsigned32(u32 a, u32 b) {
    return std::max(a, b);
}

u64 Interpreter::ExecuteMaxUnsigned64(u64 a, u64 b) {
    return std::max(a, b);
}

s32 Interpreter::ExecuteMinSigned32(s32 a, s32 b) {
    return std::min(a, b);
}

s64 Interpreter::ExecuteMinSigned64(s64 a, s64 b) {
    return std::min(a, b);
}

u32 Interpreter::ExecuteMinUnsigned32(u32 a, u32 b) {
    return std::min(a, b);
}

u64 Interpreter::ExecuteMinUnsigned64(u64 a, u64 b) {
    return std::min(a, b);
}

} // namespace Dynarmic::Backend::Interpreter
