#pragma once
#include <cstddef>

// specific layout for 2d partilce //! 2nd order
struct Particle2D_2ndOrd_StateLayout {
    static constexpr std::size_t POS_X = 0;
    static constexpr std::size_t POS_Y = 1;
    static constexpr std::size_t VEL_X = 2;
    static constexpr std::size_t VEL_Y = 3;
};

struct Particle2D_2ndOrd_DerivativeLayout {
    static constexpr std::size_t VEL_X = 0;
    static constexpr std::size_t VEL_Y = 1;
    static constexpr std::size_t ACC_X = 2;
    static constexpr std::size_t ACC_Y = 3;
};

/*
struct Particle2D_2ndOrd_SubLayout {
    struct POS {
        constexpr std::size_t X = 0;
        constexpr std::size_t Y = 1;
    };

    struct VEL {
        constexpr std::size_t X = 2;
        constexpr std::size_t Y = 3;
    };
};
*/