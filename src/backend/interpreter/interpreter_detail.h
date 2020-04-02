/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <type_traits>

#include "backend/interpreter/interpreter_context.h"
#include "backend/interpreter/value.h"
#include "common/assert.h"
#include "common/mp/function_info.h"
#include "frontend/ir/microinstruction.h"

namespace Dynarmic::Backend::Interpreter::detail {

using namespace mp = Dynarmic::Common::mp;

template <typename T, typename MFT>
inline void Execute(T this, FnT fn, InterpreterContext& ctx, IR::Inst* inst) {
    using Iota = std::make_index_sequence<mp::parameter_count_v<MFT>>;

    if constexpr (std::is_same_v<mp::equivalent_function_type_t<MFT>, void(InterpreterContext&, IR::Inst*)>) {
        (this->*fn)(ctx, inst);
    } else if constexpr (std::is_same_v<mp::return_type_t<MFT>, void>) {
        ExecuteImpl(std::forward<T>(this), std::forward<FnT>(fn), ctx, inst, Iota{});
    } else {
        ctx.value[inst].Set(ExecuteImpl(std::forward<T>(this), std::forward<FnT>(fn), ctx, inst, Iota{}));
    }
}

template <typename ArgT, size_t i>
inline ArgT GetArg(InterpreterContext& ctx, IR::Inst* inst) {
    const IR::Value arg = inst->GetArg(i);

    if constexpr (std::is_same_v<ArgT, A32::Reg>) {
        return arg.GetA32RegRef();
    } else if constexpr (std::is_same_v<ArgT, A32::ExtReg>) {
        return arg.GetA32ExtRegRef();
    } else if constexpr (std::is_same_v<ArgT, A64::Reg>) {
        return arg.GetA64RegRef();
    } else if constexpr (std::is_same_v<ArgT, A64::Vec>) {
        return arg.GetA64VecRef();
    } else if constexpr (std::is_same_v<ArgT, bool>) {
        if (!arg.IsImmediate()) {
            return ctx.value[arg.GetInst()].Get<ArgT>();
        }
        return arg.GetU1();
    } else if constexpr (std::is_same_v<ArgT, u8>) {
        if (!arg.IsImmediate()) {
            return ctx.value[arg.GetInst()].Get<ArgT>();
        }
        return arg.GetU8();
    } else if constexpr (std::is_same_v<ArgT, u16>) {
        if (!arg.IsImmediate()) {
            return ctx.value[arg.GetInst()].Get<ArgT>();
        }
        return arg.GetU16();
    } else if constexpr (std::is_same_v<ArgT, u32>) {
        if (!arg.IsImmediate()) {
            return ctx.value[arg.GetInst()].Get<ArgT>();
        }
        return arg.GetU32();
    } else if constexpr (std::is_same_v<ArgT, u64>) {
        if (!arg.IsImmediate()) {
            return ctx.value[arg.GetInst()].Get<ArgT>();
        }
        return arg.GetU64();
    } else if constexpr (std::is_same_v<ArgT, CoprocessorInfo>) {
        return arg.GetCoprocInfo();
    } else if constexpr (std::is_same_v<ArgT, Cond>) {
        return arg.GetCond();
    } else if constexpr (std::is_signed_v<ArgT>) {
        return static_cast<ArgT>(GetArg<std::make_unsigned_t<ArgT>, i>(ctx, inst));
    } else {
        if (!arg.IsImmediate()) {
            return ctx.value[arg.GetInst()].Get<ArgT>();
        }
    }
    UNREACHABLE("Impossible state reached in GetArgImpl");
}

template <typename T, typename FnT, size_t ...iota>
inline auto ExecuteImpl(T this, FnT fn, InterpreterContext& ctx, IR::Inst* inst, std::integer_sequence<size_t, iota...>) {
    return (this->*fn)(GetArg<typename mp::parameter_type_t<FnT, iota>, iota>(ctx, inst)...);
}

} // namespace Dynarmic::Backend::Interpreter::detail
