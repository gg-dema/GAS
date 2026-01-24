#pragma once
#include <cstddef>
#include <layout/Particle2D_second_order.hpp>  // predefine state layout
#include <layout/Particle2D_second_order.hpp>
namespace gas::config {

// using StateLayout = Particle2D_1ndOrd_StateLayout;
// using DerivLayout = Particle2D_1ndOrd_DerivativeLayout;
using StateLayout = Particle2D_2ndOrd_StateLayout;
using DerivLayout = Particle2D_2ndOrd_DerivativeLayout;
constexpr std::size_t MAX_PARTICLE = 10000;
constexpr int N = 4;
constexpr int CELL_SIZE = 200;
constexpr int INTEGRATION_WINDOW = 1;
constexpr bool WORLD_IS_INFINITE = true;

}  // end namespace gas::config