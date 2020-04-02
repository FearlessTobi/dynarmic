/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

#include <map>

#include "backend/interpreter/value.h"

namespace Dynarmic::IR {
class Inst;
} // namespace Dynarmic::IR

namespace Dynarmic::Backend::Interpreter {

struct ExecutionContext {
    ExecutionContext();

    std::map<IR::Inst*, Value> values;
};

} // namespace Dynarmic::Backend::Interpreter