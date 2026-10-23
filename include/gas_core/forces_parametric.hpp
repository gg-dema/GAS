#pragma once
#include <cmath>

#include "gas_core/forces.hpp"

// Parametric counterparts of the fixed-parameter forces in forces.hpp.
// The math is identical; the parameters are captured by value so that a single
// force composition can be swept over many parameter sets at run time.
// With the default arguments each factory reproduces the corresponding
// original force exactly.

// Inverse-square central attraction: acc = -GM / r^2 along the radial vector.
// Original: planetGravity_2 (G = 9.8, M = 2e6, centre at the origin).
inline auto make_central_gravity(float GM = 9.8f * 2000000.0f,
                                 float cx = 0.0f,
                                 float cy = 0.0f) {
    return [=](auto& s, auto& d) {
        float dx = s.posX() - cx;
        float dy = s.posY() - cy;
        float r2 = dx * dx + dy * dy + 1e-6f;
        float r = std::sqrt(r2);
        float acc = -GM / r2;
        d.accX() += acc * (dx / r);
        d.accY() += acc * (dy / r);
    };
}

// Tangential swirl with linear drag around a centre.
// Original: Vortex_2 (omega_base = 10000, damping_base = 0.00096, horizon = 100).
inline auto make_vortex(float omega_base = 10000.0f,
                        float damping_base = 0.00096f,
                        float horizon = 100.0f,
                        float cx = 0.0f,
                        float cy = 0.0f) {
    return [=](auto& s, auto& d) {
        float delta_x = cx - s.posX();
        float delta_y = cy - s.posY();
        float radius = std::hypot(delta_x, delta_y);

        radius = std::fmax(radius, horizon);

        float omega = omega_base / radius;
        float damping = damping_base * radius;

        d.accX() += omega * delta_y - damping * s.velX();
        d.accY() += -omega * delta_x - damping * s.velY();
    };
}

// Linear contraction of the position derivative: pdot -= k * p.
// Original: density (k = 15).
inline auto make_contraction(float k = 15.0f) {
    return [=](auto& s, auto& d) {
        d.velX() -= k * s.posX();
        d.velY() -= k * s.posY();
    };
}
