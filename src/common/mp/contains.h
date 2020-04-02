/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * This software may be used and distributed according to the terms of the GNU
 * General Public License version 2 or any later version.
 */

#pragma once

namespace Dynarmic::Common::mp {

namespace detail {

template <class L, class T>
struct contains_impl;

template <template<class...> class LT, class... Ts, class T>
struct contains_impl<LT<Ts...>, T> {
    static constexpr bool value = (false || ... || std::is_same_v<Ts, T>);
};

} // namespace detail

/// Does list L containts type T?
template <class L, class T>
constexpr bool contains = detail::contains_impl<L, T>::value;

} // namespace Dynarmic::Common::mp
