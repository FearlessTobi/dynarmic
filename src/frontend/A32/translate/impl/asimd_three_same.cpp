/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "common/bit_util.h"

#include "frontend/A32/translate/impl/translate_arm.h"

namespace Dynarmic::A32 {
namespace {
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

bool ArmTranslatorVisitor::asimd_VMUL_float(bool D, bool sz, size_t Vn, size_t Vd, bool N, bool Q, bool M, size_t Vm) {
    return FloatingPointInstruction(*this, D, sz, Vn, Vd, N, Q, M, Vm, [this](const auto&, const auto& reg_n, const auto& reg_m) {
        return ir.FPVectorMul(32, reg_n, reg_m, false);
    });
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

} // namespace Dynarmic::A32
