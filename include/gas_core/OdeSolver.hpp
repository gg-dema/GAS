#pragma once

#include <algorithm>  // for for_each
#include <cstddef>    // std:size_t
#include <ranges>     // for define std::views
#include <tuple>      // std::tuple

#include "gas_core/state_abstraction.hpp"

template <std::size_t N>
class OdeSolver {
   public:
    OdeSolver(){};
    ~OdeSolver(){};

    // force application --> generic force application
    template <typename StateLayout, typename DerivLayout, typename... Forces>
    void apply_forces(State<N>& state,
                      State<N>& derivative,
                      const std::tuple<Forces...>& forces) {
        std::fill(derivative.begin(), derivative.end(), 0.0);
        auto stateView = StateView<StateLayout, State<N>>{state};
        auto derivativeView = StateView<DerivLayout, State<N>>{derivative};

        std::apply([&](auto&&... f) { (f(stateView, derivativeView), ...); },
                   forces);
    }

    // interaction forces
    template <typename StateLayout, typename DerivLayout, typename... Forces>
    void apply_forces(State<N>& state_i,
                      State<N>& state_j,
                      State<N>& derivative_i,
                      State<N>& derivative_j,
                      const std::tuple<Forces...>& forces) {
        auto state_i_View = StateView<StateLayout, State<N>>{state_i};
        auto derivative_i_View = StateView<DerivLayout, State<N>>{derivative_i};

        auto state_j_View = StateView<StateLayout, State<N>>{state_j};
        auto derivative_j_View = StateView<DerivLayout, State<N>>{derivative_j};

        std::apply(
            [&](auto&&... f) {
                (f(state_i_View,
                   state_j_View,
                   derivative_i_View,
                   derivative_j_View),
                 ...);
            },
            forces);
    }

    State<N> euler_integration(const State<N>& x,
                               const State<N>& x_dot,
                               float delta_t) {
        State<N> x_next;

        auto indices = std::views::iota(std::size_t{0}, N);
        std::ranges::for_each(indices, [&](std::size_t i) {
            x_next[i] = x[i] + x_dot[i] * delta_t;
        });

        return x_next;
    }
};