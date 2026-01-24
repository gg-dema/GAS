#include <config.hpp>

#include "simulator.cpp"

template <int N, typename... Forces>
auto makeSimulator(
    Forces&&... fs) {  // Forces&& -> forward reference: accept both r/l-val
    using Sim = Simulator<N, std::decay_t<Forces>...>;
    // the forward is used for move the object if lvalue, or copy if rvalue
    return Sim(std::forward<Forces>(fs)...);
}
