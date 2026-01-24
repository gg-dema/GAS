#pragma once
#include <state_abstraction.hpp>

template <int N>
class IOdeSolver {
    template <typename StateLayout, typename DerivLayout, typename... Forces>

    // used for indipendent forces (eg: const gravity, linear attractor)
    virtual void apply_forces(const State<N>&,
                              State<N>&,
                              const std::tuple<Forces...>&) = 0;

    // used for pairwise interaction forces (eg: ollision, N-body gravity)
    virtual void apply_forces(const State<N>&,
                              const State<N>&,
                              State<N>&,
                              State<N>&,
                              const std::tuple<Forces...>&) = 0;

    virtual State<N> integrate(const State<N>&,
                               const State<N>&,
                               float delta_t) = 0;
};
