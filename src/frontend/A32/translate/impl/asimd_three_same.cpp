/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {
namespace {
enum class Comparison {
    GE,
    GT,
    EQ,
};

template <bool WithDst, typename Callable>
bool BitwiseInstruction(ArmTranslatorVisitor& v, bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm, Callable fn) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    if constexpr (WithDst) {
        const IR::U128 reg_d = v.ir.GetVector(d);
        const IR::U128 reg_m = v.ir.GetVector(m);
        const IR::U128 reg_n = v.ir.GetVector(n);
        const IR::U128 result = fn(reg_d, reg_n, reg_m);
        v.ir.SetVector(d, result);
    } else {
        const IR::U128 reg_m = v.ir.GetVector(m);
        const IR::U128 reg_n = v.ir.GetVector(n);
        const IR::U128 result = fn(reg_n, reg_m);
        v.ir.SetVector(d, result);
    }

    return true;
}

template <typename Callable>
bool FloatingPointInstruction(ArmTranslatorVisitor& v, bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm, Callable fn) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    if (sz == 0b1) {
        return v.UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_d = v.ir.GetVector(d);
    const auto reg_n = v.ir.GetVector(n);
    const auto reg_m = v.ir.GetVector(m);
    const auto result = fn(reg_d, reg_n, reg_m);

    v.ir.SetVector(d, result);
    return true;
}

bool IntegerComparison(ArmTranslatorVisitor& v, bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm,
                       Comparison comparison) {
    if (sz == 0b11) {
        return v.UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = v.ir.GetVector(n);
    const auto reg_m = v.ir.GetVector(m);
    const auto result = [&] {
        switch (comparison) {
        case Comparison::GT:
            return U ? v.ir.VectorGreaterUnsigned(esize, reg_n, reg_m)
                     : v.ir.VectorGreaterSigned(esize, reg_n, reg_m);
        case Comparison::GE:
            return U ? v.ir.VectorGreaterEqualUnsigned(esize, reg_n, reg_m)
                     : v.ir.VectorGreaterEqualSigned(esize, reg_n, reg_m);
        case Comparison::EQ:
            return v.ir.VectorEqual(esize, reg_n, reg_m);
        default:
            return IR::U128{};
        }
    }();

    v.ir.SetVector(d, result);
    return true;
}

bool FloatComparison(ArmTranslatorVisitor& v, bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm,
                     Comparison comparison) {
    if (sz) {
        return v.UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return v.UndefinedInstruction();
    }

    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = v.ir.GetVector(n);
    const auto reg_m = v.ir.GetVector(m);
    const auto result = [&] {
        switch (comparison) {
        case Comparison::GE:
            return v.ir.FPVectorGreaterEqual(32, reg_n, reg_m, false);
        case Comparison::GT:
            return v.ir.FPVectorGreater(32, reg_n, reg_m, false);
        case Comparison::EQ:
            return v.ir.FPVectorEqual(32, reg_n, reg_m, false);
        default:
            return IR::U128{};
        }
    }();

    v.ir.SetVector(d, result);
    return true;
}
} // Anonymous namespace

bool ArmTranslatorVisitor::asimd_VHADD(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const IR::U128 reg_n = ir.GetVector(n);
    const IR::U128 reg_m = ir.GetVector(m);
    const IR::U128 result = U ? ir.VectorHalvingAddUnsigned(esize, reg_n, reg_m) : ir.VectorHalvingAddSigned(esize, reg_n, reg_m);
    ir.SetVector(d, result);

    return true;
}

bool ArmTranslatorVisitor::asimd_VQADD(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const IR::U128 reg_n = ir.GetVector(n);
    const IR::U128 reg_m = ir.GetVector(m);
    const IR::U128 result = U ? ir.VectorUnsignedSaturatedAdd(esize, reg_n, reg_m) : ir.VectorSignedSaturatedAdd(esize, reg_n, reg_m);
    ir.SetVector(d, result);

    return true;
}

bool ArmTranslatorVisitor::asimd_VRHADD(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const IR::U128 reg_n = ir.GetVector(n);
    const IR::U128 reg_m = ir.GetVector(m);
    const IR::U128 result = U ? ir.VectorRoundingHalvingAddUnsigned(esize, reg_n, reg_m) : ir.VectorRoundingHalvingAddSigned(esize, reg_n, reg_m);
    ir.SetVector(d, result);

    return true;
}

bool ArmTranslatorVisitor::asimd_VAND_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<false>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.VectorAnd(reg_n, reg_m);
    });
}

bool ArmTranslatorVisitor::asimd_VBIC_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<false>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.VectorAnd(reg_n, ir.VectorNot(reg_m));
    });
}

bool ArmTranslatorVisitor::asimd_VORR_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<false>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.VectorOr(reg_n, reg_m);
    });
}

bool ArmTranslatorVisitor::asimd_VORN_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<false>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.VectorOr(reg_n, ir.VectorNot(reg_m));
    });
}

bool ArmTranslatorVisitor::asimd_VEOR_reg(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<false>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_n, const auto& reg_m) {
        return ir.VectorEor(reg_n, reg_m);
    });
}

bool ArmTranslatorVisitor::asimd_VBSL(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<true>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        return ir.VectorOr(ir.VectorAnd(reg_n, reg_d), ir.VectorAnd(reg_m, ir.VectorNot(reg_d)));
    });
}

bool ArmTranslatorVisitor::asimd_VBIT(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<true>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        return ir.VectorOr(ir.VectorAnd(reg_n, reg_m), ir.VectorAnd(reg_d, ir.VectorNot(reg_m)));
    });
}

bool ArmTranslatorVisitor::asimd_VBIF(bool D, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return BitwiseInstruction<true>(*this, D, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        return ir.VectorOr(ir.VectorAnd(reg_d, reg_m), ir.VectorAnd(reg_n, ir.VectorNot(reg_m)));
    });
}

bool ArmTranslatorVisitor::asimd_VHSUB(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const IR::U128 reg_n = ir.GetVector(n);
    const IR::U128 reg_m = ir.GetVector(m);
    const IR::U128 result = U ? ir.VectorHalvingSubUnsigned(esize, reg_n, reg_m) : ir.VectorHalvingSubSigned(esize, reg_n, reg_m);
    ir.SetVector(d, result);

    return true;
}

bool ArmTranslatorVisitor::asimd_VQSUB(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const IR::U128 reg_n = ir.GetVector(n);
    const IR::U128 reg_m = ir.GetVector(m);
    const IR::U128 result = U ? ir.VectorUnsignedSaturatedSub(esize, reg_n, reg_m) : ir.VectorSignedSaturatedSub(esize, reg_n, reg_m);
    ir.SetVector(d, result);

    return true;
}

bool ArmTranslatorVisitor::asimd_VCGT_reg(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return IntegerComparison(*this, U, D, sz, Vn, Vd, N, Q, M, Vm, Comparison::GT);
}

bool ArmTranslatorVisitor::asimd_VCGE_reg(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return IntegerComparison(*this, U, D, sz, Vn, Vd, N, Q, M, Vm, Comparison::GE);
}

bool ArmTranslatorVisitor::asimd_VADD_int(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);
    const auto result = ir.VectorAdd(esize, reg_m, reg_n);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VSUB_int(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);
    const auto result = ir.VectorSub(esize, reg_n, reg_m);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VSHL_reg(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);
    const auto result = U ? ir.VectorLogicalVShift(esize, reg_m, reg_n)
                          : ir.VectorArithmeticVShift(esize, reg_m, reg_n);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VQSHL_reg(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);
    const auto result = U ? ir.VectorUnsignedSaturatedShiftLeft(esize, reg_m, reg_n)
                          : ir.VectorSignedSaturatedShiftLeft(esize, reg_m, reg_n);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VRSHL(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);
    const auto result = U ? ir.VectorRoundingShiftLeftUnsigned(esize, reg_m, reg_n)
                          : ir.VectorRoundingShiftLeftSigned(esize, reg_m, reg_n);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VMAX(bool U, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, bool op, size_t Vm) {
    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_m = ir.GetVector(m);
    const auto reg_n = ir.GetVector(n);
    const auto result = [&] {
        if (op) {
            return U ? ir.VectorMinUnsigned(esize, reg_m, reg_n)
                     : ir.VectorMinSigned(esize, reg_m, reg_n);
        } else {
            return U ? ir.VectorMaxUnsigned(esize, reg_m, reg_n)
                     : ir.VectorMaxSigned(esize, reg_m, reg_n);
        }
    }();

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VTST(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8 << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto anded = ir.VectorAnd(reg_n, reg_m);
    const auto result = ir.VectorNot(ir.VectorEqual(esize, anded, ir.ZeroVector()));

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VCEQ_reg(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return IntegerComparison(*this, false, D, sz, Vn, Vd, N, Q, M, Vm, Comparison::EQ);
}

bool ArmTranslatorVisitor::asimd_VMLA(bool op, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (sz == 0b11) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto reg_d = ir.GetVector(d);
    const auto multiply = ir.VectorMultiply(esize, reg_m, reg_n);
    const auto result = op ? ir.VectorSub(esize, reg_d, multiply)
                           : ir.VectorAdd(esize, reg_d, multiply);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VMUL(bool P, bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (sz == 0b11 || (P && sz != 0b00)) {
        return UndefinedInstruction();
    }

    if (Q && (Common::Bit<0>(Vd) || Common::Bit<0>(Vn) || Common::Bit<0>(Vm))) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto result = P ? ir.VectorPolynomialMultiply(reg_m, reg_n)
                          : ir.VectorMultiply(esize, reg_m, reg_n);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VPADD(bool D, size_t sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    if (Q || sz == 0b11) {
        return UndefinedInstruction();
    }

    const size_t esize = 8U << sz;
    const auto d = ToVector(Q, Vd, D);
    const auto m = ToVector(Q, Vm, M);
    const auto n = ToVector(Q, Vn, N);

    const auto reg_n = ir.GetVector(n);
    const auto reg_m = ir.GetVector(m);
    const auto result = ir.VectorPairedAddLower(esize, reg_n, reg_m);

    ir.SetVector(d, result);
    return true;
}

bool ArmTranslatorVisitor::asimd_VADD_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorAdd(32, reg_n, reg_m, false);
    });
}

bool ArmTranslatorVisitor::asimd_VSUB_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorSub(32, reg_n, reg_m, false);
    });
}

bool ArmTranslatorVisitor::asimd_VPADD_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this, Q](const auto&, const auto& reg_n, const auto& reg_m) {
        return Q ? ir.FPVectorPairedAdd(32, reg_n, reg_m, false)
                 : ir.FPVectorPairedAddLower(32, reg_n, reg_m, false);
    });
}

bool ArmTranslatorVisitor::asimd_VABD_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorAbs(32, ir.FPVectorSub(32, reg_n, reg_m, false));
    });
}

bool ArmTranslatorVisitor::asimd_VMLA_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        const auto product = ir.FPVectorMul(32, reg_n, reg_m, false);
        return ir.FPVectorAdd(32, reg_d, product, false);
    });
}

bool ArmTranslatorVisitor::asimd_VMLS_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto& reg_d, const auto& reg_n, const auto& reg_m) {
        const auto product = ir.FPVectorMul(32, reg_n, reg_m, false);
        return ir.FPVectorAdd(32, reg_d, ir.FPVectorNeg(32, product), false);
    });
}

bool ArmTranslatorVisitor::asimd_VMUL_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorMul(32, reg_n, reg_m, false);
    });
}

bool ArmTranslatorVisitor::asimd_VCEQ_reg_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatComparison(*this, D, sz, Vn, Vd, N, Q, M, Vm, Comparison::EQ);
}

bool ArmTranslatorVisitor::asimd_VCGE_reg_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatComparison(*this, D, sz, Vn, Vd, N, Q, M, Vm, Comparison::GE);
}

bool ArmTranslatorVisitor::asimd_VCGT_reg_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatComparison(*this, D, sz, Vn, Vd, N, Q, M, Vm, Comparison::GT);
}

bool ArmTranslatorVisitor::asimd_VMAX_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorMax(32, reg_n, reg_m, false);
    });
}

bool ArmTranslatorVisitor::asimd_VMIN_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorMin(32, reg_n, reg_m, false);
    });
}

bool ArmTranslatorVisitor::asimd_VRECPS(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorRecipStepFused(32, reg_n, reg_m, false);
    });
}

bool ArmTranslatorVisitor::asimd_VRSQRTS(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorRSqrtStepFused(32, reg_n, reg_m, false);
    });
}

} // namespace Dynarmic::A32
