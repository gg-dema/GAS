#include <algorithm>
#include <cmath>
#include <ranges>
#include <vector>

#include "config.hpp"
#include "gas_core/state_abstraction.hpp"
#include "visual/particle.hpp"

extern int HEIGHT;
extern int WIDTH;

class Bridge {
   public:
    std::vector<gas::visual::Particle2D>
        buffers[2];  // array of type std::vector<?>
    int readIndex = 0;
    int writeIndex = 1;

    Bridge() {}
    void swapBuffers() { std::swap(readIndex, writeIndex); }
    void set_physics_data(
        const std::vector<State<gas::config::N>>& state_vector) {
        // copy the physics field that i want and add it as renderable
        // particle in my buffer
        float x_min, x_max, y_min, y_max;
        y_min = -HEIGHT;  // 2.0f;
        y_max = HEIGHT;   // 2.0f;
        x_min = -WIDTH;   // 2.0f;
        x_max = WIDTH;    // 2.0f;

        auto filtered_range =
            state_vector |
            std::views::filter([x_min, x_max, y_min, y_max](auto const& s) {
                return s.data[0] > x_min && s.data[0] < x_max &&
                       s.data[1] > y_min && s.data[1] < y_max;
            });

        std::vector<State<gas::config::N>> valid_particles(
            std::ranges::begin(filtered_range),
            std::ranges::end(filtered_range));
        // std::vector<State<gas::config::N>> valid_particles(state_vector);
        buffers[writeIndex].clear();
        buffers[writeIndex].reserve(valid_particles.size());
        // buffers[writeIndex].reserve(state_vector.size());

        std::vector<float> vel;
        vel.resize(valid_particles.size());

        std::transform(valid_particles.begin(),
                       valid_particles.end(),
                       vel.begin(),
                       [](const auto& p) { return std::hypot(p[2], p[3]); });
        auto [min_it, max_it] = std::minmax_element(vel.begin(), vel.end());
        float min_vel = *min_it;
        float max_vel = *max_it;
        float inv_range = 1.0F / (max_vel - min_vel + 1e-5);

        for (size_t i = 0; i < valid_particles.size(); ++i) {
            const auto& p = valid_particles[i];
            float v_norm = (vel[i] - min_vel) * inv_range;

            gas::visual::Particle2D particle{
                {p[0], p[1]}, {v_norm, 0.0f, 1.0f - v_norm}, 10.0F, 1.0F};

            buffers[writeIndex].emplace_back(particle);
        }
    }
    const std::vector<gas::visual::Particle2D>& get_render_data() const {
        return buffers[readIndex];
    }
};
