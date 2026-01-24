#pragma once

#include <array>
#include <cstddef>

struct StateParameters {
    float mass;
};
template <std::size_t N>
struct State {
    std::array<float, N> data{};
    StateParameters params;

    float& operator[](std::size_t i) { return data[i]; }
    const float& operator[](std::size_t i) const { return data[i]; }

    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
};

// state view (first attemp, later on generalize)
template <typename Layout, typename StateType>
struct StateView {
    StateType& s;
    // ps: this will not compile only if i try to access layout::Acc from some
    // state that do not have it
    float& posX() { return s[Layout::POS_X]; }
    float& posY() { return s[Layout::POS_Y]; }
    float& velX() { return s[Layout::VEL_X]; }
    float& velY() { return s[Layout::VEL_Y]; }
    float& accX() { return s[Layout::ACC_X]; }
    float& accY() { return s[Layout::ACC_Y]; }
};

/*
template <typename SubLayout, typename StateType>
struct SubStateView {
    StateType& s;
    std::array<float, N>& {}
    float& X() { return s[SubLayout::X]; }
    float& Y() { return s[SubLayout::Y]; }
    float& Z() { return s[SubLayout::Z]; }
}
    */