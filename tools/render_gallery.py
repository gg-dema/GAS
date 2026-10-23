#!/usr/bin/env python3
"""Render the output of gas_headless into figures.

Reads <runs>/<scenario>/{meta.json,metrics.csv,step_*.csv} and writes

  <out>/<scenario>.png        time strip: the saved snapshots of one scenario
  <out>/contact_sheet.png     final state of every scenario with its metrics
  <out>/showcase.png          the selected scenarios in one figure (README)
  <out>/sweep_*.png           parameter sweeps, final state side by side
  <out>/validation.png        pure contraction against the analytical decay
  <out>/ranking.txt           scenarios ranked by simple shape metrics

With --docs DIR the figures used by the README are also copied to DIR.

Particles are coloured with the same mapping as the OpenGL front end
(Bridge::set_physics_data): normalised speed v in [0, 1] -> RGB (v, 0, 1 - v).
"""

import argparse
import json
import math
import re
import shutil
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402

BG = "#000000"
FG = "#b8b8b8"
POINT = 1.4

# Scenarios shown in the README, in order.
CURATED = [
    "app_baseline",
    "app_forces_no_collisions",
    "gravity_swirl",
    "orbit_ring_eccentric",
    "van_der_pol_disc",
    "counter_vortices",
]

# (scenario, title, subtitle) shown in the showcase figure, row by row.
SHOWCASE = [
    ("app_baseline", "Reference configuration", "shift + gravity + vortex + contraction"),
    ("app_forces_no_collisions", "Symmetric collapse", "same forces, collisions off"),
    ("swirl_k_40", "Strong contraction", "k = 40, omega_base = 10000"),
    ("swirl_omega_30000", "Strong swirl", "k = 15, omega_base = 30000"),
    ("gravity_swirl", "Gravity and swirl", "GM = 1.96e7, omega_base = 3000"),
    ("orbit_ring_eccentric", "Eccentric orbits", "ring at 0.6 of circular speed"),
    ("van_der_pol_disc", "Decoupled axes", "Van der Pol oscillator, rotating disc"),
    ("counter_vortices", "Vortex dipole", "omega_base = +8000 and -8000"),
]

# name -> (title, [(scenario, panel label), ...])
SWEEPS = {
    "sweep_swirl_strength": (
        "Swirl strength omega_base (contraction k = 15, horizon = 100)",
        [("swirl_omega_1000", "omega_base = 1000"), ("swirl_omega_3000", "omega_base = 3000"),
         ("swirl_omega_10000", "omega_base = 10000"), ("swirl_omega_30000", "omega_base = 30000")]),
    "sweep_contraction_rate": (
        "Contraction rate k (omega_base = 10000, horizon = 100)",
        [("swirl_k_3", "k = 3"), ("swirl_k_6", "k = 6"),
         ("swirl_omega_10000", "k = 15"), ("swirl_k_40", "k = 40")]),
    "sweep_vortex_core": (
        "Vortex core radius (k = 15, omega_base = 10000)",
        [("swirl_horizon_30", "horizon = 30"), ("swirl_omega_10000", "horizon = 100"),
         ("swirl_horizon_300", "horizon = 300")]),
}


def load_snapshot(path):
    return np.loadtxt(path, delimiter=",", skiprows=1)


def speed_colors(vel):
    speed = np.hypot(vel[:, 0], vel[:, 1])
    v = (speed - speed.min()) / (speed.max() - speed.min() + 1e-5)
    return np.stack([v, np.zeros_like(v), 1.0 - v], axis=1)


def extent(pos):
    r = np.hypot(pos[:, 0], pos[:, 1])
    return 1.1 * max(float(np.percentile(r, 99)), 1.0)


def draw_panel(ax, snap, title):
    pos, vel = snap[:, :2], snap[:, 2:]
    ax.set_facecolor(BG)
    ax.scatter(pos[:, 0], pos[:, 1], s=POINT, c=speed_colors(vel), linewidths=0)
    lim = extent(pos)
    ax.set_xlim(-lim, lim)
    ax.set_ylim(-lim, lim)
    ax.set_aspect("equal")
    ax.tick_params(colors=FG, labelsize=8, length=2)
    for side in ax.spines.values():
        side.set_color("#333333")
    ax.set_title(title, color=FG, fontsize=10)


def read_metrics(path):
    return np.genfromtxt(path, delimiter=",", names=True)


def scenario_dirs(runs):
    return sorted(d for d in runs.iterdir() if (d / "meta.json").exists())


def snapshots_of(d):
    return sorted(d.glob("step_*.csv"))


def render_strip(d, meta, out):
    files = snapshots_of(d)
    if not files:
        return None
    if len(files) == 5:  # drop the earliest transient frame, keep four panels
        files = [files[0]] + files[2:]
    fig, axes = plt.subplots(1, len(files), figsize=(3.5 * len(files), 4.3), facecolor=BG)
    axes = np.atleast_1d(axes)
    for ax, f in zip(axes, files):
        step = int(f.stem.split("_")[1])
        draw_panel(ax, load_snapshot(f), f"t = {step * meta['dt']:.3f} s   step {step}")
    fig.suptitle(meta["name"], color="#e0e0e0", fontsize=13, y=0.985)
    fig.text(0.5, 0.012, meta["forces"], color=FG, fontsize=9, ha="center")
    fig.tight_layout(rect=(0, 0.045, 1, 0.95))
    path = out / f"{meta['name']}.png"
    fig.savefig(path, dpi=130, facecolor=BG)
    plt.close(fig)
    return path


def summarise(d, meta):
    m = read_metrics(d / "metrics.csv")
    m = np.atleast_1d(m)
    first, last = m[0], m[-1]
    contraction = 1.0 - last["mean_r"] / max(first["mean_r"], 1e-9)
    symmetry = last["aniso"] / (1.0 + last["ang_cv"])
    scale = max(last["mean_r"] * last["speed"], 1e-9)
    swirl = abs(last["lz"]) / scale
    ring = 1.0 - (last["r90"] - last["r50"]) / max(last["r90"], 1e-9)
    return dict(name=meta["name"], contraction=contraction, symmetry=symmetry,
                swirl=swirl, ring=ring, final_r=last["mean_r"],
                aniso=last["aniso"], ang_cv=last["ang_cv"])


def render_contact_sheet(rows, runs, out):
    n = len(rows)
    cols = min(6, n)
    nrows = math.ceil(n / cols)
    fig, axes = plt.subplots(nrows, cols, figsize=(3.0 * cols, 3.3 * nrows), facecolor=BG)
    axes = np.atleast_1d(axes).ravel()
    for ax, row in zip(axes, rows):
        files = snapshots_of(runs / row["name"])
        draw_panel(ax, load_snapshot(files[-1]), row["name"])
        ax.set_xlabel(
            f"sym {row['symmetry']:.2f}  contr {row['contraction']:.2f}  swirl {row['swirl']:.2f}",
            color=FG, fontsize=6)
    for ax in axes[n:]:
        ax.axis("off")
    fig.tight_layout()
    fig.savefig(out / "contact_sheet.png", dpi=110, facecolor=BG)
    plt.close(fig)


def write_ranking(rows, out):
    lines = []
    for title, key in (("symmetric collapse", lambda r: r["symmetry"] * r["contraction"]),
                       ("rotating structure", lambda r: r["swirl"]),
                       ("ring like", lambda r: r["ring"] * r["final_r"])):
        lines.append(f"# {title}")
        for r in sorted(rows, key=key, reverse=True):
            lines.append(f"  {r['name']:<28s} score {key(r):8.3f}  final_r {r['final_r']:8.1f}"
                         f"  aniso {r['aniso']:.2f}  ang_cv {r['ang_cv']:.2f}")
        lines.append("")
    (out / "ranking.txt").write_text("\n".join(lines))


def render_showcase(runs, out, cols=4):
    if not all((runs / name / "meta.json").exists() for name, _, _ in SHOWCASE):
        return None
    rows = math.ceil(len(SHOWCASE) / cols)
    fig, axes = plt.subplots(rows, cols, figsize=(3.6 * cols, 4.0 * rows), facecolor=BG)
    axes = np.atleast_1d(axes).ravel()
    for ax, (name, title, sub) in zip(axes, SHOWCASE):
        meta = json.loads((runs / name / "meta.json").read_text())
        snap = load_snapshot(snapshots_of(runs / name)[-1])
        draw_panel(ax, snap, title)
        ax.tick_params(labelsize=7)
        ax.set_xlabel(f"{sub}\nt = {meta['steps'] * meta['dt']:g} s", color=FG, fontsize=8)
    for ax in axes[len(SHOWCASE):]:
        ax.axis("off")
    fig.tight_layout(h_pad=1.2)
    path = out / "showcase.png"
    fig.savefig(path, dpi=120, facecolor=BG)
    plt.close(fig)
    return path


def render_sweep(runs, out, key):
    title, items = SWEEPS[key]
    if not all((runs / name / "meta.json").exists() for name, _ in items):
        return None
    fig, axes = plt.subplots(1, len(items), figsize=(3.5 * len(items), 4.3), facecolor=BG)
    for ax, (name, label) in zip(axes, items):
        snap = load_snapshot(snapshots_of(runs / name)[-1])
        draw_panel(ax, snap, label)
        ax.set_xlabel(f"mean radius {np.hypot(snap[:, 0], snap[:, 1]).mean():.0f}",
                      color=FG, fontsize=9)
    fig.suptitle(title, color="#e0e0e0", fontsize=13, y=0.985)
    fig.tight_layout(rect=(0, 0, 1, 0.95))
    path = out / f"{key}.png"
    fig.savefig(path, dpi=130, facecolor=BG)
    plt.close(fig)
    return path


def render_validation(runs, out):
    d = runs / "pure_contraction"
    if not (d / "meta.json").exists():
        return None
    meta = json.loads((d / "meta.json").read_text())
    k = float(re.search(r"make_contraction\(([\d.]+)\)", meta["forces"]).group(1))
    m = read_metrics(d / "metrics.csv")
    n = np.round(m["t"] / meta["dt"])
    discrete = m["mean_r"][0] * (1.0 - k * meta["dt"]) ** n
    analytic = m["mean_r"][0] * np.exp(-k * m["t"])
    fig, ax = plt.subplots(figsize=(6.4, 3.8), facecolor="white")
    ax.semilogy(m["t"], m["mean_r"], "-", lw=2.4, color="#1f5fbf", label="simulation (mean radius)")
    ax.semilogy(m["t"], discrete, "--", lw=1.4, color="#d04020",
                label=r"explicit Euler: $r_0 (1 - k\,\Delta t)^n$")
    ax.semilogy(m["t"], analytic, ":", lw=1.4, color="#202020", label=r"continuous: $r_0 e^{-kt}$")
    ax.set_xlabel("time [s]")
    ax.set_ylabel("mean radius")
    ax.set_title(f"Linear contraction, k = {k:g} (eigenvalue -{k:g})", fontsize=10)
    ax.grid(True, which="both", alpha=0.25)
    ax.legend(fontsize=8)
    fig.tight_layout()
    fig.savefig(out / "validation.png", dpi=150)
    plt.close(fig)
    err = float(np.max(np.abs(m["mean_r"] - discrete) / discrete))
    print(f"pure_contraction: max relative deviation from discrete solution = {err:.2e}")
    return err


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--runs", type=Path, default=Path("build/runs"))
    ap.add_argument("--out", type=Path, default=None)
    ap.add_argument("--docs", type=Path, default=None,
                    help="also copy the README figures to this directory")
    args = ap.parse_args()
    out = args.out or args.runs / "pictures"
    out.mkdir(parents=True, exist_ok=True)

    rows = []
    for d in scenario_dirs(args.runs):
        meta = json.loads((d / "meta.json").read_text())
        path = render_strip(d, meta, out)
        if path is None:
            continue
        print("wrote", path)
        if meta["name"] != "pure_contraction":
            rows.append(summarise(d, meta))
    if rows:
        render_contact_sheet(rows, args.runs, out)
        write_ranking(rows, out)
    path = render_showcase(args.runs, out)
    if path:
        print("wrote", path)
    for key in SWEEPS:
        path = render_sweep(args.runs, out, key)
        if path:
            print("wrote", path)
    render_validation(args.runs, out)

    if args.docs:
        args.docs.mkdir(parents=True, exist_ok=True)
        wanted = [f"{n}.png" for n in CURATED] + [f"{k}.png" for k in SWEEPS] + [
            "showcase.png", "validation.png"]
        for name in wanted:
            if (out / name).exists():
                shutil.copy(out / name, args.docs / name)
                print("copied", args.docs / name)


if __name__ == "__main__":
    main()
