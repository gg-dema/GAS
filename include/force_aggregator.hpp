/*
Author : Gabriele G.G. Dema Di Marzo
mail: gg.dema.rm.22@gmail.com
---------------------------------------
*/

#include <tuple>

template <typename... Forces>
auto force_aggregator(Forces&&... f) {
    return std::make_tuple(std::move(f)...);
}
