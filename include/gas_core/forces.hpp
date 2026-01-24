#pragma once
#include <cmath>

inline auto second_order_shift = [](auto& s, auto& d) {
    d.velX() += s.velX();
    d.velY() += s.velY();
};

inline auto const_gravity = [](auto& s, auto& d) { d.accY() += -10; };

inline auto linear_attractor_1stOrd = [](auto& s, auto& d) {
    d.velX() = (250 - s.posX());
    d.velY() = (250 - s.posY());
    // d.accX() = 5.0F * (20.0F - s.velX());
    // d.accY() = 5.0F * (20.0F - s.velY());
};

inline auto VanDerPol_oscillator = [](auto& s, auto& d) {
    float mu = 0.00005;
    d.accX() = mu * (10.0 - s.posX() * s.posX()) * s.velX() - s.posX();
    d.accY() = mu * (10.0 - s.posY() * s.posY()) * s.velY() - s.posY();
};

inline auto linear_attractor_2 = [](auto& s, auto& d) {
    // d.accX() = 0.1F * (-s.posX());
    // d.accY() = 0.1F * (-s.posY());
};

inline auto Vortex_1 = [](auto& s, auto& d) {
    float planet_coord_x = 400;
    float planet_coord_y = 100;
    float omega_base = 500;  // angular speed
    float damping_base = 0.0001F;

    float delta_x = planet_coord_x - s.posX();
    float delta_y = planet_coord_y - s.posY();
    float radius = std::hypot(delta_x, delta_y);
    float horizon = 50.F;

    radius = std::fmax(radius, horizon);

    float omega = omega_base / radius;
    float damping = damping_base * radius;

    d.accX() += omega * delta_y - damping * s.velX();
    d.accY() += -omega * delta_x - damping * s.velY();
};
inline auto Vortex_2 = [](auto& s, auto& d) {
    float planet_coord_x = 0;
    float planet_coord_y = 0;
    float omega_base = 10000;  // angular speed
    float damping_base = 0.00096F;

    float delta_x = planet_coord_x - s.posX();
    float delta_y = planet_coord_y - s.posY();
    float radius = std::hypot(delta_x, delta_y);
    float horizon = 100.F;

    radius = std::fmax(radius, horizon);

    float omega = omega_base / radius;
    float damping = damping_base * radius;

    d.accX() += omega * delta_y - damping * s.velX();
    d.accY() += -omega * delta_x - damping * s.velY();
};

inline auto planetGravity_2 = [](auto& s, auto& d) {
    float G = 9.8f;
    float M = 2000000.0f;
    float planet_X = 0;
    float planet_Y = 0;
    float dx = s.posX() - planet_X;
    float dy = s.posY() - planet_Y;
    float r2 = dx * dx + dy * dy + 1e-6f;
    float r = std::sqrt(r2);
    float acc = -G * M / r2;
    d.accX() += acc * (dx / r);
    d.accY() += acc * (dy / r);
};

inline auto density = [](auto& s, auto& d) {
    float density = 15.0;
    d.velX() -= density * s.posX();
    d.velY() -= density * s.posY();
    // d.accX() -= s.velX();
    // d.accY() -= s.velY();
};

inline auto collision_FORCE = [](auto& s1, auto& s2, auto& d1, auto& d2) {
    constexpr float RADIUS = 30.0f;
    constexpr float MIN_DIST = 2.0f * RADIUS;
    constexpr float STIFFNESS = 1.0f;   // spring-like repulsion
    constexpr float DAMPING = 1000.0f;  // velocity damping

    float dx = s1.posX() - s2.posX();
    float dy = s1.posY() - s2.posY();
    float dist2 = dx * dx + dy * dy;

    if (dist2 < MIN_DIST * MIN_DIST && dist2 > 1e-8f) {
        float dist = std::sqrt(dist2);
        float nx = dx / dist;  // collision normal x
        float ny = dy / dist;  // collision normal y

        // Overlap penetration depth
        float overlap = MIN_DIST - dist;

        // Relative velocity
        float rvx = s1.velX() - s2.velX();
        float rvy = s1.velY() - s2.velY();
        float relVelAlongNormal = rvx * nx + rvy * ny;

        // Repulsion force (spring + damping)
        float force = STIFFNESS * overlap - DAMPING * relVelAlongNormal;

        // Apply equal and opposite accelerations
        d1.accX() += force * nx;
        d1.accY() += force * ny;
        d2.accX() -= force * nx;
        d2.accY() -= force * ny;
    }
};

extern int WIDTH;
extern int HEIGHT;

inline auto collision_box = [](auto& s, auto& d) {
    constexpr float RADIUS = 30.0F;
    float xmin = -WIDTH / 2;
    float xmax = WIDTH / 2;
    float ymin = -HEIGHT / 2;
    float ymax = HEIGHT / 2;

    if (s.posX() < xmin + RADIUS) {
        d.accX() = -s.velX();
    }
    if (s.posX() > xmax - RADIUS) {
        d.accX() = -s.velX();
    }
    if (s.posY() < ymin + RADIUS) {
        d.accY() = -s.velY();
    }
    if (s.posY() > ymax - RADIUS) {
        d.accY() = -s.velY();
    }
};