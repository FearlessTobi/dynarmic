/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#include "backend/interpreter/interpreter.h"
#include "backend/interpreter/interpreter_context.h"
#include "frontend/ir/microinstruction.h"
#include "frontend/ir/value.h"

namespace Dynarmic::Backend::Interpreter {

Interpreter::Interpreter() = default;
Interpreter::~Interpreter() = default;

void Interpreter::ExecuteVoid() {
    // nop
}

void Interpreter::ExecuteIdentity(ExecutionContext& ctx, IR::Inst* inst) {
    const IR::Value arg = inst->GetArg(0);

    if (!arg.IsImmediate()) {
        ctx.values[inst] = ctx.values[arg.GetInst()];
        return;
    }

    switch (arg.GetType()) {
        case IR::Type::Void:
        case IR::Type::A32Reg:
        case IR::Type::A32ExtReg:
        case IR::Type::A64Reg:
        case IR::Type::A64Vec:
        case IR::Type::Opaque:
        case IR::Type::CoprocInfo:
        case IR::Type::Table:
        case IR::Type::NZCVFlags:
        case IR::Type::Cond:
            UNREACHABLE_MSG("Invalid Identity immediate type");
            return;
        case IR::Type::U1:
            ctx.values[inst].Set(arg.GetU1());
            return;
        case IR::Type::U8:
            ctx.values[inst].Set(arg.GetU8());
            return;
        case IR::Type::U16:
            ctx.values[inst].Set(arg.GetU16());
            return;
        case IR::Type::U32:
            ctx.values[inst].Set(arg.GetU32());
            return;
        case IR::Type::U64:
            ctx.values[inst].Set(arg.GetU64());
            return;
        case IR::Type::U128:
            UNREACHABLE_MSG("U128 not a valid immediate type");
            return;
        default:
            UNREACHABLE_MSG("Invalid IR::Value type");
            return;
    }
}

void Interpreter::ExecuteBreakpoint() {
    UNREACHABLE_MSG("Stray IR breakpoint instruction");
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Hints
///////////////////////////////////////////////////////////////////////////////////////////////////

void Interpreter::ExecutePushRSB(u64) {
    // nop : ignore hint
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Pseudo-operations
///////////////////////////////////////////////////////////////////////////////////////////////////

void Interpreter::ExecuteGetCarryFromOp(ExecutionContext& ctx, IR::Inst* inst) {
    const IR::Value arg = inst->GetArg(0);
    ctx.values[inst].Set(ctx.values[arg.GetInst()].GetCarry());
}

void Interpreter::ExecuteGetOverflowFromOp(ExecutionContext& ctx, IR::Inst* inst) {
    const IR::Value arg = inst->GetArg(0);
    ctx.values[inst].Set(ctx.values[arg.GetInst()].GetOverflow());
}

u32 Interpreter::ExecuteGetGEFromOp(ResultAndGE<u32> value) {
    return value.ge;
}

NZCV Interpreter::ExecuteGetNZCVFromOp(ResultAndNZCV<u32> value) {
    u32 nzcv = 0;
    nzcv |= (value.negative ? 1 : 0) << 31;
    nzcv |= (value.zero ? 1 : 0) << 30;
    nzcv |= (value.carry ? 1 : 0) << 29;
    nzcv |= (value.overflow ? 1 : 0) << 28;
    return nzcv;
}

Vector Interpreter::ExecuteGetUpperFromOp(UpperAndLower value) {
    return value.upper;
}

Vector Interpreter::ExecuteGetLowerFromOp(UpperAndLower value) {
    return value.lower;
}

NZCV Interpreter::ExecuteNZCVFromPackedFlags(u32 value) {
    return value;
}

} // namespace Dynarmic::Backend::Interpreter
