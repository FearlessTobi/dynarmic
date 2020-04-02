/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <array>
#include <type_traits>
#include <variant>

#include "common/bit_util.h"
#include "common/cast_util.h"
#include "common/common_types.h"

namespace Dynarmic::Backend::Interpreter {

template <typename T>
using VectorOf = std::array<T, 128 / Common::BitSize<T>()>;
using Vector = std::array<u64, 2>;

template <typename T>
struct ResultAndCarry {
    T result;
    bool carry;
};

template <typename T>
struct ResultAndOverflow {
    T result;
    bool overflow;
};

template <typename T>
struct ResultAndCarryAndOverflow {
    T result;
    bool carry;
    bool overflow;
};

template <typename T>
struct ResultAndNZCV {
    T result;
    bool negative;
    bool zero;
    bool carry;
    bool overflow;
};

template <typename T>
struct ResultAndGE {
    T result;
    u32 ge;
};

struct UpperAndLower {
    Vector upper;
    Vector lower;
};

struct Value {
    template <typename T>
    T Get() const {
        if constexpr (sizeof(T) == sizeof(Vector)) {
            return Common::BitCast<T>(std::get<Vector>(value));
        } else if constexpr (std::is_signed_v<T>) {
            using U = std::make_unsigned_t<T>;
            return static_cast<T>(Get<U>());
        } else {
            if constexpr (std::is_same_v<T, u32>) {
                if (const auto v = std::get_if<ResultAndCarry<T>>(&value)) {
                    return v->result;
                } else if (const auto v = std::get_if<ResultAndCarryAndOverflow<T>>(&value)) {
                    return v->result;
                } else if (const auto v = std::get_if<ResultAndGE<T>>(&value)) {
                    return v->result;
                } else if (const auto v = std::get_if<ResultAndNZCV<T>>(&value)) {
                    return v->result;
                }
            }
            if constexpr (std::is_same_v<T, u8> || std::is_same_v<T, u16> || std::is_same_v<T, u32> || std::is_same_v<T, u64>) {
                if (const auto v = std::get_if<ResultAndOverflow<T>>(&value)) {
                    return v->result;
                }
            }

            return std::get<T>(value);
        }
    }

    template <typename T>
    void Set(T new_value) {
        if constexpr (sizeof(T) == sizeof(Vector)) {
            value = Common::BitCast<Vector>(new_value);
        } else if constexpr (std::is_signed_v<T>) {
            using U = std::make_unsigned_t<T>;
            Set<U>(static_cast<U>(new_value));
        } else {
            value = new_value;
        }
    }

    bool GetCarry() const {
        if (const auto v = std::get_if<ResultAndCarry<u32>>(&value)) {
            return v->carry;
        } else if (const auto v = std::get_if<ResultAndCarryAndOverflow<u32>>(&value)) {
            return v->carry;
        } else if (const auto v = std::get_if<ResultAndNZCV<u32>>(&value)) {
            return v->carry;
        }
        throw std::bad_variant_access();
    }

    bool GetOverflow() const {
        if (const auto v = std::get_if<ResultAndOverflow<u8>>(&value)) {
            return v->overflow;
        } else if (const auto v = std::get_if<ResultAndOverflow<u16>>(&value)) {
            return v->overflow;
        } else if (const auto v = std::get_if<ResultAndOverflow<u32>>(&value)) {
            return v->overflow;
        } else if (const auto v = std::get_if<ResultAndOverflow<u64>>(&value)) {
            return v->overflow;
        } else if (const auto v = std::get_if<ResultAndCarryAndOverflow<u32>>(&value)) {
            return v->overflow;
        } else if (const auto v = std::get_if<ResultAndNZCV<u32>>(&value)) {
            return v->overflow;
        }
        throw std::bad_variant_access();
    }

private:
    using Variant = std::variant<std::monostate, 
        bool, u8, u16, u32, u64, Vector,
        ResultAndCarry<u32>,
        ResultAndCarryAndOverflow<u32>,
        ResultAndGE<u32>,
        ResultAndNZCV<u32>,
        ResultAndOverflow<u8>, ResultAndOverflow<u16>, ResultAndOverflow<u32>, ResultAndOverflow<u64>,
        UpperAndLower
    >;
    Variant value;
};

} // namespace Dynarmic::Backend::Interpreter
