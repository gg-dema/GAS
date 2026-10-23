// clang-format off
#include <gas_core/state_abstraction.hpp>
#include <gas_core/OdeSolver.hpp>
#include <gas_core/spatial_hasing.hpp>
#include <config.hpp>

// ! TODO URGENT: collision is not passed at compilation time, other forces are templated
#include <gas_core/forces.hpp>

#include <vector>
#include <algorithm>    // ranges::generate_n
#include <random>       // random init of the particle
// clang-format on

extern int WIDTH;
extern int HEIGHT;

template <int N, typename... Forces>
class Simulator {
    using StateLayout = gas::config::StateLayout;
    using DerivLayout = gas::config::DerivLayout;

   public:
    std::vector<State<N>> _state{gas::config::MAX_PARTICLE},
        _next_state{gas::config::MAX_PARTICLE};
    std::vector<State<N>> _derivative{gas::config::MAX_PARTICLE};
    std::tuple<Forces...> _forces;
    OdeSolver<N> _solver;
    Grid<N> _spatial_hash_grid;
    // pairwise collision response; on by default, can be disabled to study the
    // pure single-particle ODE (see src/headless)
    bool collisions_enabled = true;

    Simulator(Forces... f) : _forces(std::make_tuple(std::move(f)...)) {}

    ~Simulator() {}

    void random_initialization() {
        std::random_device rand_dev;
        std::mt19937 generator(rand_dev());
        std::uniform_real_distribution<float> distrib_x(-WIDTH, WIDTH);
        std::uniform_real_distribution<float> distrib_y(-HEIGHT, HEIGHT);
        std::uniform_real_distribution<float> distrib_vel(0.0, 0.0);

        auto generator_state_funct = [&]() {
            State<N> s{};
            for (int i = 0; i < N / 2; i += 2) {
                s.data[i] = distrib_x(generator);
                s.data[i + 1] = distrib_y(generator);
            }

            for (int i = N / 2; i < N; i++) {
                s.data[i] = distrib_vel(generator);
            }

            return s;
        };
        std::ranges::generate_n(
            _state.begin(), gas::config::MAX_PARTICLE, generator_state_funct);
    }

    void evolve(int N_step) {
        for (int i = 0; i < N_step; i++) {
            if (collisions_enabled) {
                _spatial_hash_grid.fill(_state);
            }
            for (int particle_idx = 0; particle_idx < gas::config::MAX_PARTICLE;
                 particle_idx++) {
                float x = _state[particle_idx].data[0];
                float y = _state[particle_idx].data[1];
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
                // fill spatial hashing grid for collision detection:

                // apply generic forces
                _solver.template apply_forces<StateLayout, DerivLayout>(
                    _state[particle_idx], _derivative[particle_idx], _forces);
            }
            // apply collision detection and correction
            //*
            // (the grid is empty when collisions are disabled)
            for (auto& [key, particle_ids] : _spatial_hash_grid.cells) {
                auto pair = _spatial_hash_grid.get_pairs(particle_ids);
                for (auto& [idx_i, idx_j] : pair) {
                    _solver.template apply_forces<StateLayout, DerivLayout>(
                        _state[idx_i],
                        _state[idx_j],
                        _derivative[idx_i],
                        _derivative[idx_j],
                        std::make_tuple(collision_FORCE));
                }
            }
            //*/

            // apply pseudo N-body
            for (int particle_idx = 0; particle_idx < gas::config::MAX_PARTICLE;
                 particle_idx++) {
                _next_state[particle_idx] = _solver.template euler_integration(
                    _state[particle_idx], _derivative[particle_idx], 0.001);
            }
            std::swap(_state, _next_state);
            _spatial_hash_grid.reset();
        }
    }
};
