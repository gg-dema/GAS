#pragma once

#include <array>
#include <cmath>
#include <config.hpp>
#include <gas_core/state_abstraction.hpp>
#include <unordered_map>
#include <vector>

extern int HEIGHT, WIDTH;

template <int WS_DIM>
struct GridKey {
    std::array<float, WS_DIM> position;

    bool operator==(const GridKey& other) const {
        bool equals = true;
        for (int i = 0; i < WS_DIM; i++) {
            if (equals) {
                equals = (position[i] == other.position[i]);
            }
        }
        return equals;
    }
};

template <int WS_DIM>
struct GridHashFunction {
    std::size_t operator()(const GridKey<WS_DIM>& key) const {
        std::size_t seed = 0UL;
        std::hash<float> hasher{};
        for (int i = 0; i < WS_DIM; i++) {
            hash_combiner(seed, hasher(key.position[i]));
        }
        return seed;
    }

   private:
    static inline void hash_combiner(std::size_t& seed,
                                     const std::size_t& value) {
        seed ^= (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    };
};

template <int WS_DIM>
class Grid {
   public:
    Grid(){};
    Grid(const std::vector<State<WS_DIM>>& state_vectors) {
        fill(state_vectors);
    }

    void fill(const std::vector<State<WS_DIM>>& state_vectors) {
        for (int i = 0; i < state_vectors.size(); ++i) {
            // TODO : ugly fix for infinite world --> try to avoid access data
            // // in this manner
            float x = state_vectors[i].data[0];
            float y = state_vectors[i].data[1];
            if (!gas::config::WORLD_IS_INFINITE) {
                // box in "window space"
                float xmin = -WIDTH / 2.0f;
                float xmax = WIDTH / 2.0f;
                float ymin = -HEIGHT / 2.0f;
                float ymax = HEIGHT / 2.0f;

                // skip particles outside the visible box
                if (x < xmin || x > xmax || y < ymin || y > ymax) {
                    continue;
                }
            }
            auto key = get_key(state_vectors[i]);
            cells[key].push_back(i);
        }
    }
    void reset() { cells.clear(); }

    std::unordered_map<GridKey<WS_DIM>,
                       std::vector<int>,
                       GridHashFunction<WS_DIM>>
        cells;

    GridKey<WS_DIM> get_key(const State<WS_DIM>& state) {
        GridKey<WS_DIM> k{};
        for (int i = 0; i < WS_DIM; i++) {
            k.position[i] = std::floor(state.data[i] / gas::config::CELL_SIZE);
        }
        return k;
    }

    std::vector<std::pair<int, int>> get_pairs(
        const std::vector<int>& indexs) const {
        std::vector<std::pair<int, int>> out;
        out.reserve(indexs.size() * (indexs.size() - 1) / 2);

        for (size_t i = 0; i < indexs.size(); ++i) {
            for (size_t j = i + 1; j < indexs.size(); ++j) {
                out.emplace_back(indexs[i], indexs[j]);
            }
        }
        return out;
    }
};