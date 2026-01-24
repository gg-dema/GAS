#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "config.hpp"
#include "gas_core/forces.hpp"
#include "gas_core/simulator.cpp"
#include "visual/bridge_v2.cpp"
#include "visual/rendering_engine.hpp"

class App {
   public:
    Bridge bridge;

    Simulator<gas::config::N,
              decltype(second_order_shift),
              decltype(planetGravity_2),
              decltype(Vortex_2),
              decltype(density)
              //   decltype(const_gravity)
              >
        simulator{
            second_order_shift, planetGravity_2, Vortex_2, density
            //   const_gravity
        };
    gas::visual::RenderingEngine renderer;
    std::thread physicsThread;
    std::atomic<bool> running{true};

    App() { simulator.random_initialization(); }

    void physics_loop() {
        while (running) {
            simulator.evolve(gas::config::INTEGRATION_WINDOW);
            bridge.set_physics_data(simulator._state);
            bridge.swapBuffers();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    void run() {
        // starting simulation thread
        physicsThread = std::thread(&App::physics_loop, this);

        using clock = std::chrono::high_resolution_clock;
        auto lastTime = clock::now();
        int frames = 0;

        // rendering thread
        while (!glfwWindowShouldClose(renderer.window)) {
            auto frameStart = clock::now();

            auto particles = bridge.get_render_data();
            renderer.step(particles);

            frames++;
            if (std::chrono::duration<double>(frameStart - lastTime).count() >=
                1.0) {  // if delta_t >= 1 second
                std::cout << "FPS: " << frames << "\n";
                frames = 0;
                lastTime = frameStart;
            }
        }

        // close all
        running = false;
        physicsThread.join();
    }
};
