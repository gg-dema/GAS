#pragma once
#include <cstddef>

/*
 *  Layout for first order 2d particle
 */

struct Particle2D_1ndOrd_StateLayout {
    static constexpr std::size_t POS_X = 0;
    static constexpr std::size_t POS_Y = 1;
};

struct Particle2D_1ndOrd_DerivativeLayout {
    static constexpr std::size_t VEL_X = 0;
    static constexpr std::size_t VEL_Y = 1;
};
/*
struct Particle2D_1ndOrd_SubLayout {
    struct POS {
        constexpr std::size_t X = 0;
        constexpr std::size_t Y = 1;
    };
};*/