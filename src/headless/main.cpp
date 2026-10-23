// Headless driver: runs the gas simulator without a window and dumps particle
// snapshots plus time-series metrics to disk. Intended for parameter sweeps and
// for generating the figures in the README (see tools/render_gallery.py).
//
//   gas_headless [--out DIR] [--only NAME] [--list] [--selftest]

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "gas_core/forces_parametric.hpp"
#include "gas_core/simulator.cpp"

int WIDTH = 1000;
int HEIGHT = 500;

namespace {

namespace fs = std::filesystem;

// Mirrors the integration step hard-coded in Simulator::evolve.
constexpr double kDt = 0.001;

enum class Init { Box, Disc, Ring };

struct Run {
    std::string name;
    std::string forces;  // human readable composition, stored in meta.json
    Init init = Init::Disc;
    float radius = 700.0f;      // disc radius, or ring centre radius
    float half_width = 0.0f;    // ring half width
    float tangential = 0.0f;    // initial counter-clockwise tangential speed
    bool collisions = true;
    unsigned seed = 1;
    int steps = 2000;
    int log_every = 10;
    std::vector<int> snapshots;  // step indices, multiples of log_every
};

const char* init_name(Init i) {
    switch (i) {
        case Init::Box: return "box";
        case Init::Disc: return "disc";
        case Init::Ring: return "ring";
    }
    return "?";
}

template <class Sim>
void initialise(Sim& sim, const Run& r) {
    std::mt19937 gen(r.seed);
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);
    const float two_pi = 2.0f * std::numbers::pi_v<float>;

    for (auto& s : sim._state) {
        float x = 0.0f, y = 0.0f, vx = 0.0f, vy = 0.0f;
        if (r.init == Init::Box) {
            float ux = u01(gen);
            float uy = u01(gen);
            x = (2.0f * ux - 1.0f) * static_cast<float>(WIDTH);
            y = (2.0f * uy - 1.0f) * static_cast<float>(HEIGHT);
        } else {
            float ur = u01(gen);
            float ut = u01(gen);
            float rad = (r.init == Init::Disc)
                            ? r.radius * std::sqrt(ur)
                            : r.radius + r.half_width * (2.0f * ur - 1.0f);
            float th = two_pi * ut;
            x = rad * std::cos(th);
            y = rad * std::sin(th);
            vx = -r.tangential * std::sin(th);
            vy = r.tangential * std::cos(th);
        }
        s.data = {x, y, vx, vy};
    }
}

struct Metrics {
    double mean_r = 0, r50 = 0, r90 = 0;
    double aniso = 1;   // sqrt(lambda_min / lambda_max) of second moments, 1 = isotropic
    double ang_cv = 0;  // coefficient of variation of the 36-bin angular histogram
    double lz = 0;      // mean angular momentum x*vy - y*vx
    double speed = 0;   // mean speed
    std::size_t bad = 0;
};

template <class Sim>
Metrics measure(const Sim& sim) {
    Metrics m;
    const auto& st = sim._state;
    const std::size_t n = st.size();
    std::vector<double> radii;
    radii.reserve(n);
    double sxx = 0, syy = 0, sxy = 0;
    std::array<double, 36> hist{};
    std::size_t good = 0;

    for (const auto& s : st) {
        double x = s[0], y = s[1], vx = s[2], vy = s[3];
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(vx) ||
            !std::isfinite(vy)) {
            ++m.bad;
            continue;
        }
        ++good;
        double r = std::hypot(x, y);
        radii.push_back(r);
        m.mean_r += r;
        sxx += x * x;
        syy += y * y;
        sxy += x * y;
        m.lz += x * vy - y * vx;
        m.speed += std::hypot(vx, vy);
        double a = std::atan2(y, x);  // [-pi, pi]
        int bin = static_cast<int>((a + std::numbers::pi) /
                                   (2.0 * std::numbers::pi) * 36.0);
        hist[std::clamp(bin, 0, 35)] += 1.0;
    }
    if (good == 0) return m;

    const double inv = 1.0 / static_cast<double>(good);
    m.mean_r *= inv;
    m.lz *= inv;
    m.speed *= inv;
    sxx *= inv;
    syy *= inv;
    sxy *= inv;

    double half_tr = 0.5 * (sxx + syy);
    double disc = std::sqrt(0.25 * (sxx - syy) * (sxx - syy) + sxy * sxy);
    double lmax = half_tr + disc, lmin = half_tr - disc;
    m.aniso = lmax > 0 ? std::sqrt(std::max(lmin, 0.0) / lmax) : 1.0;

    double hm = 0;
    for (double h : hist) hm += h;
    hm /= 36.0;
    double var = 0;
    for (double h : hist) var += (h - hm) * (h - hm);
    var /= 36.0;
    m.ang_cv = hm > 0 ? std::sqrt(var) / hm : 0.0;

    auto kth = [&](double q) {
        std::size_t k = static_cast<std::size_t>(q * (radii.size() - 1));
        std::nth_element(radii.begin(), radii.begin() + k, radii.end());
        return radii[k];
    };
    m.r50 = kth(0.5);
    m.r90 = kth(0.9);
    return m;
}

template <class Sim>
void write_snapshot(const Sim& sim, const fs::path& path) {
    std::ofstream f(path);
    f << "x,y,vx,vy\n" << std::setprecision(9);
    for (const auto& s : sim._state) {
        f << s[0] << ',' << s[1] << ',' << s[2] << ',' << s[3] << '\n';
    }
}

void write_meta(const Run& r, const fs::path& path) {
    std::ofstream f(path);
    f << "{\n"
      << "  \"name\": \"" << r.name << "\",\n"
      << "  \"forces\": \"" << r.forces << "\",\n"
      << "  \"init\": \"" << init_name(r.init) << "\",\n"
      << "  \"radius\": " << r.radius << ",\n"
      << "  \"half_width\": " << r.half_width << ",\n"
      << "  \"tangential\": " << r.tangential << ",\n"
      << "  \"collisions\": " << (r.collisions ? "true" : "false") << ",\n"
      << "  \"seed\": " << r.seed << ",\n"
      << "  \"steps\": " << r.steps << ",\n"
      << "  \"dt\": " << kDt << ",\n"
      << "  \"particles\": " << gas::config::MAX_PARTICLE << ",\n"
      << "  \"snapshots\": [";
    for (std::size_t i = 0; i < r.snapshots.size(); ++i) {
        f << (i ? ", " : "") << r.snapshots[i];
    }
    f << "]\n}\n";
}

// Runs one scenario. Returns false if the state became non-finite.
template <class... Forces>
bool run_scenario(const fs::path& out_root, Run r, Forces... forces) {
    using clock = std::chrono::steady_clock;
    if (r.snapshots.empty()) {
        // initial state plus four later frames, spaced to catch fast transients
        for (double f : {0.0, 1.0 / 30.0, 1.0 / 10.0, 1.0 / 3.0, 1.0}) {
            int step = static_cast<int>(f * r.steps) / r.log_every * r.log_every;
            if (r.snapshots.empty() || step != r.snapshots.back()) r.snapshots.push_back(step);
        }
    }
    const fs::path dir = out_root / r.name;
    fs::create_directories(dir);
    write_meta(r, dir / "meta.json");

    Simulator<gas::config::N, Forces...> sim(std::move(forces)...);
    sim.collisions_enabled = r.collisions;
    initialise(sim, r);

    const std::set<int> snaps(r.snapshots.begin(), r.snapshots.end());
    std::ofstream mf(dir / "metrics.csv");
    mf << "step,t,mean_r,r50,r90,aniso,ang_cv,lz,speed\n" << std::setprecision(9);

    auto emit = [&](int step) {
        Metrics m = measure(sim);
        if (m.bad > 0) return false;
        mf << step << ',' << step * kDt << ',' << m.mean_r << ',' << m.r50 << ','
           << m.r90 << ',' << m.aniso << ',' << m.ang_cv << ',' << m.lz << ','
           << m.speed << '\n';
        if (snaps.count(step)) {
            char name[32];
            std::snprintf(name, sizeof(name), "step_%06d.csv", step);
            write_snapshot(sim, dir / name);
        }
        return true;
    };

    if (!emit(0)) return false;
    auto t_chunk = clock::now();
    int last_snap = 0;
    for (int step = r.log_every; step <= r.steps; step += r.log_every) {
        sim.evolve(r.log_every);
        if (!emit(step)) {
            std::printf("[%s] NON-FINITE state at step %d, scenario aborted\n",
                        r.name.c_str(), step);
            return false;
        }
        if (snaps.count(step)) {
            auto now = clock::now();
            double ms = std::chrono::duration<double, std::milli>(now - t_chunk).count();
            std::printf("[%s] step %6d  mean_r %9.3f  (%.2f ms/step since last snapshot)\n",
                        r.name.c_str(), step, measure(sim).mean_r,
                        ms / (step - last_snap));
            t_chunk = now;
            last_snap = step;
        }
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    fs::path out = "runs";
    std::string only;
    bool list = false, selftest = false;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--out") && i + 1 < argc) out = argv[++i];
        else if (!std::strcmp(argv[i], "--only") && i + 1 < argc) only = argv[++i];
        else if (!std::strcmp(argv[i], "--list")) list = true;
        else if (!std::strcmp(argv[i], "--selftest")) selftest = true;
    }

    if (selftest) {
        // The parametric factories with default arguments must reproduce the
        // fixed-parameter forces of forces.hpp exactly (bit for bit).
        Run r;
        r.init = Init::Box;
        Simulator<gas::config::N, decltype(second_order_shift), decltype(planetGravity_2),
                  decltype(Vortex_2), decltype(density)>
            a(second_order_shift, planetGravity_2, Vortex_2, density);
        auto g = make_central_gravity();
        auto v = make_vortex();
        auto c = make_contraction();
        Simulator<gas::config::N, decltype(second_order_shift), decltype(g), decltype(v),
                  decltype(c)>
            b(second_order_shift, g, v, c);
        initialise(a, r);
        initialise(b, r);
        a.evolve(200);
        b.evolve(200);
        std::size_t diff = 0;
        for (std::size_t i = 0; i < a._state.size(); ++i) {
            if (std::memcmp(a._state[i].data.data(), b._state[i].data.data(),
                            sizeof(float) * gas::config::N) != 0) {
                ++diff;
            }
        }
        std::printf("selftest: %zu of %zu particles differ after 200 steps\n", diff,
                    a._state.size());
        return diff == 0 ? 0 : 1;
    }

    std::vector<std::pair<std::string, std::function<void()>>> table;
    auto add = [&](std::string name, std::function<void()> fn) {
        table.emplace_back(std::move(name), std::move(fn));
    };

    auto base = [](const char* name, const char* forces, Init init, int steps) {
        Run r;
        r.name = name;
        r.forces = forces;
        r.init = init;
        r.steps = steps;
        return r;
    };
    const float GM0 = 9.8f * 2000000.0f;  // the gravity constant used by planetGravity_2

    // --- Reference configuration -------------------------------------------
    // The exact composition used by src/app.cpp, original fixed-parameter lambdas.
    add("app_baseline", [&] {
        Run r = base("app_baseline",
                     "second_order_shift + planetGravity_2 + Vortex_2 + density", Init::Box, 3000);
        r.snapshots = {0, 100, 300, 1000, 3000};
        run_scenario(out, r, second_order_shift, planetGravity_2, Vortex_2, density);
    });

    // Same composition on a disc, with and without collisions.
    add("app_forces_disc", [&] {
        Run r = base("app_forces_disc",
                     "second_order_shift + planetGravity_2 + Vortex_2 + density, disc", Init::Disc, 3000);
        run_scenario(out, r, second_order_shift, planetGravity_2, Vortex_2, density);
    });
    add("app_forces_no_collisions", [&] {
        Run r = base("app_forces_no_collisions",
                     "second_order_shift + planetGravity_2 + Vortex_2 + density, collisions off",
                     Init::Disc, 3000);
        r.collisions = false;
        run_scenario(out, r, second_order_shift, planetGravity_2, Vortex_2, density);
    });

    // --- Analytical check ---------------------------------------------------
    // pdot = v - k p with v = 0 gives p(t) = p0 (1 - k dt)^n for explicit Euler.
    add("pure_contraction", [&] {
        Run r = base("pure_contraction", "second_order_shift + make_contraction(15)", Init::Disc, 400);
        r.collisions = false;
        r.log_every = 1;
        r.snapshots = {0, 50, 100, 400};
        run_scenario(out, r, second_order_shift, make_contraction(15.0f));
    });

    // --- Contraction (stable linear system) ---------------------------------
    add("contraction_packed", [&] {
        Run r = base("contraction_packed", "second_order_shift + make_contraction(15), collisions on",
                     Init::Disc, 1500);
        run_scenario(out, r, second_order_shift, make_contraction(15.0f));
    });

    // --- Contraction plus swirl: sweep over the swirl strength ---------------
    for (float omega : {1000.0f, 3000.0f, 10000.0f, 30000.0f}) {
        std::string name = "swirl_omega_" + std::to_string(static_cast<int>(omega));
        add(name, [&, name, omega] {
            Run r = base(name.c_str(), "second_order_shift + make_contraction(15) + make_vortex(omega)",
                         Init::Disc, 3000);
            r.forces = "second_order_shift + make_contraction(15) + make_vortex(" +
                       std::to_string(static_cast<int>(omega)) + ", 0.00096, 100)";
            run_scenario(out, r, second_order_shift, make_contraction(15.0f),
                         make_vortex(omega, 0.00096f, 100.0f));
        });
    }

    // --- Contraction plus swirl: sweep over the contraction rate -------------
    for (float k : {3.0f, 6.0f, 40.0f}) {
        std::string name = "swirl_k_" + std::to_string(static_cast<int>(k));
        add(name, [&, name, k] {
            Run r = base(name.c_str(), "", Init::Disc, 3000);
            r.forces = "second_order_shift + make_contraction(" + std::to_string(static_cast<int>(k)) +
                       ") + make_vortex(10000, 0.00096, 100)";
            run_scenario(out, r, second_order_shift, make_contraction(k),
                         make_vortex(10000.0f, 0.00096f, 100.0f));
        });
    }

    // --- Contraction plus swirl: sweep over the vortex core (horizon) --------
    for (float horizon : {30.0f, 300.0f}) {
        std::string name = "swirl_horizon_" + std::to_string(static_cast<int>(horizon));
        add(name, [&, name, horizon] {
            Run r = base(name.c_str(), "", Init::Disc, 3000);
            r.forces = "second_order_shift + make_contraction(15) + make_vortex(10000, 0.00096, " +
                       std::to_string(static_cast<int>(horizon)) + ")";
            run_scenario(out, r, second_order_shift, make_contraction(15.0f),
                         make_vortex(10000.0f, 0.00096f, horizon));
        });
    }

    // --- Central gravity ----------------------------------------------------
    add("free_fall", [&] {
        Run r = base("free_fall", "second_order_shift + make_central_gravity(GM0), collisions on",
                     Init::Disc, 4000);
        run_scenario(out, r, second_order_shift, make_central_gravity(GM0));
    });
    add("gravity_swirl", [&] {
        Run r = base("gravity_swirl",
                     "second_order_shift + make_central_gravity(GM0) + make_vortex(3000, 0.00096, 100)",
                     Init::Disc, 4000);
        r.radius = 600;
        run_scenario(out, r, second_order_shift, make_central_gravity(GM0),
                     make_vortex(3000.0f, 0.00096f, 100.0f));
    });

    // Circular and eccentric orbits: initial tangential speed against sqrt(GM / r).
    add("orbit_ring_circular", [&] {
        Run r = base("orbit_ring_circular",
                     "second_order_shift + make_central_gravity(GM0), ring at v = sqrt(GM0 / r0)",
                     Init::Ring, 8000);
        r.radius = 300;
        r.half_width = 30;
        r.tangential = std::sqrt(GM0 / r.radius);
        run_scenario(out, r, second_order_shift, make_central_gravity(GM0));
    });
    add("orbit_ring_circular_free", [&] {
        Run r = base("orbit_ring_circular_free",
                     "second_order_shift + make_central_gravity(GM0), ring at v = sqrt(GM0 / r0), no collisions",
                     Init::Ring, 8000);
        r.radius = 300;
        r.half_width = 30;
        r.tangential = std::sqrt(GM0 / r.radius);
        r.collisions = false;
        run_scenario(out, r, second_order_shift, make_central_gravity(GM0));
    });
    add("orbit_ring_eccentric", [&] {
        Run r = base("orbit_ring_eccentric",
                     "second_order_shift + make_central_gravity(GM0), ring at v = 0.6 sqrt(GM0 / r0), no collisions",
                     Init::Ring, 8000);
        r.radius = 300;
        r.half_width = 30;
        r.tangential = 0.6f * std::sqrt(GM0 / r.radius);
        r.collisions = false;
        run_scenario(out, r, second_order_shift, make_central_gravity(GM0));
    });
    add("orbit_disc_rotating", [&] {
        Run r = base("orbit_disc_rotating",
                     "second_order_shift + make_central_gravity(GM0), rotating disc, no collisions",
                     Init::Disc, 8000);
        r.radius = 400;
        r.tangential = 150;
        r.collisions = false;
        run_scenario(out, r, second_order_shift, make_central_gravity(GM0));
    });

    // --- Two wells and two vortices (breaks the radial symmetry) -------------
    add("binary_wells", [&] {
        Run r = base("binary_wells",
                     "second_order_shift + two make_central_gravity(GM0 / 2) at (-300, 0) and (300, 0), no collisions",
                     Init::Disc, 6000);
        r.radius = 500;
        r.tangential = 80;
        r.collisions = false;
        run_scenario(out, r, second_order_shift, make_central_gravity(GM0 / 2, -300.0f, 0.0f),
                     make_central_gravity(GM0 / 2, 300.0f, 0.0f));
    });
    add("counter_vortices", [&] {
        Run r = base("counter_vortices",
                     "second_order_shift + make_contraction(4) + make_vortex(+8000) at (-350, 0) + make_vortex(-8000) at (350, 0)",
                     Init::Disc, 4000);
        run_scenario(out, r, second_order_shift, make_contraction(4.0f),
                     make_vortex(8000.0f, 0.00096f, 100.0f, -350.0f, 0.0f),
                     make_vortex(-8000.0f, 0.00096f, 100.0f, 350.0f, 0.0f));
    });

    // --- Van der Pol style nonlinear damping ----------------------------------
    add("van_der_pol_disc", [&] {
        Run r = base("van_der_pol_disc", "second_order_shift + VanDerPol_oscillator, rotating disc, no collisions",
                     Init::Disc, 6000);
        r.tangential = 300;
        r.collisions = false;
        run_scenario(out, r, second_order_shift, VanDerPol_oscillator);
    });
    add("van_der_pol_box", [&] {
        Run r = base("van_der_pol_box", "second_order_shift + VanDerPol_oscillator, box, no collisions",
                     Init::Box, 6000);
        r.collisions = false;
        run_scenario(out, r, second_order_shift, VanDerPol_oscillator);
    });

    if (list) {
        for (auto& [name, fn] : table) std::printf("%s\n", name.c_str());
        return 0;
    }
    for (auto& [name, fn] : table) {
        if (!only.empty() && only != name) continue;
        fn();
    }
    return 0;
}
