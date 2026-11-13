#!/usr/bin/env python3
"""Рисунок «объём, число шагов и время кадра от N» для п. 3.5 статьи.

Три панели в логарифмических осях:
  а) объём структуры, МиБ (сфера – сплошные линии, ландшафт – штриховые);
  б) среднее число шагов на луч (SVDAG совпадает с SVO);
  в) время кадра, мс, для сферы: точка – среднее по ракурсам, отрезок – от минимума до максимума.
Объём плотной сетки при N > 2048 расчётный (незакрашенный маркер).

Нужен matplotlib:  pip install matplotlib

    python plot_scaling.py review_main.csv --time review_time.csv --out figure1.tif --preview

Первый CSV даёт объём, шаги и время плотной сетки; --time (необязательно) – время блочной
сетки, SVO и SVDAG из отдельного прогона (иначе берётся из первого файла). SVO и SVDAG –
со спуском со стеком, если в файле есть оба варианта.
"""

from __future__ import annotations

import argparse
import csv
import statistics
import sys
from collections import defaultdict
from pathlib import Path

CM = 1 / 2.54
MIB = 1024.0 * 1024.0

# Категориальная палитра (проверена на различимость при дальтонизме); контраст жёлтого,
# бирюзового и розового с белым фоном ниже 3:1, поэтому у каждой серии свой маркер.
STYLE = {
    "dense":       {"label": "плотная сетка",            "color": "#2a78d6", "marker": "o"},
    "dense-brick": {"label": "плотная сетка, кирпичи 8³", "color": "#e87ba4", "marker": "D"},
    "block":       {"label": "блочная сетка",            "color": "#eb6834", "marker": "s"},
    "svo":         {"label": "SVO",                      "color": "#1baf7a", "marker": "^"},
    "svdag":       {"label": "SVDAG",                    "color": "#eda100", "marker": "v"},
}
SCENES = {"sphere": ("сфера", "-"), "terrain": ("ландшафт", "--")}


def read_rows(path: Path) -> list[dict]:
    with path.open(newline="", encoding="utf-8-sig") as file:
        return list(csv.DictReader(file, delimiter=";"))


def collect(rows: list[dict]):
    """(scene, rep) -> {N: [строки ракурсов]}; для SVO/SVDAG – только stack, если он есть."""
    has_stack = any(r.get("traversal") == "stack" for r in rows)
    data: dict[tuple[str, str], dict[int, list[dict]]] = defaultdict(lambda: defaultdict(list))
    for r in rows:
        rep = r["representation"]
        if rep in ("svo", "svdag") and has_stack and r.get("traversal") != "stack":
            continue
        data[(r["scene"], rep)][int(r["N"])].append(r)
    return data


def series(data, scene: str, rep: str, field: str, ok_only: bool):
    points = []
    for n, rows in sorted(data.get((scene, rep), {}).items()):
        values = [float(r[field]) for r in rows if (r["gpu_status"] == "ok" or not ok_only) and float(r[field]) > 0]
        if values:
            points.append((n, statistics.fmean(values), min(values), max(values),
                           rows[0]["gpu_status"].startswith("skipped")))
    return points


def setup_axis(ax, ylabel: str, ns: list[int]):
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xticks(ns)
    ax.set_xticklabels([str(n) for n in ns])
    ax.minorticks_off()
    ax.set_xlabel("$N$")
    ax.set_ylabel(ylabel)
    ax.grid(True, which="major", color="#e4e3df", linewidth=0.5)
    ax.set_axisbelow(True)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)
    for side in ("left", "bottom"):
        ax.spines[side].set_linewidth(0.6)
    ax.tick_params(width=0.6, length=2.5)


def plot_line(ax, points, rep: str, linestyle: str, *, ranges: bool = False):
    if not points:
        return
    style = STYLE[rep]
    ns = [p[0] for p in points]
    ys = [p[1] for p in points]
    ax.plot(ns, ys, linestyle=linestyle, color=style["color"], linewidth=1.2, zorder=2)
    for n, y, lo, hi, computed in points:
        ax.plot([n], [y], marker=style["marker"], markersize=3.6, color=style["color"],
                markerfacecolor="white" if computed else style["color"], markeredgewidth=0.8,
                linestyle="none", zorder=3)
        if ranges and hi / lo > 1.05:
            ax.plot([n, n], [lo, hi], color=style["color"], linewidth=0.8, zorder=1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("csv", type=Path, help="CSV основного прогона (объём, шаги, время плотной сетки)")
    parser.add_argument("--time", type=Path, help="CSV прогона времени для блочной сетки, SVO и SVDAG")
    parser.add_argument("--time-scene", default="sphere", choices=list(SCENES), help="сцена на панели времени")
    parser.add_argument("--width-cm", type=float, default=17.0)
    parser.add_argument("--height-cm", type=float, default=5.6)
    parser.add_argument("--dpi", type=int, default=300)
    parser.add_argument("--font", default="Times New Roman")
    parser.add_argument("--font-size", type=float, default=9.0)
    parser.add_argument("--out", type=Path, default=Path("figure1.tif"))
    parser.add_argument("--preview", action="store_true", help="дополнительно сохранить PNG")
    args = parser.parse_args()

    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.lines import Line2D

    plt.rcParams.update({
        "font.family": "serif",
        "font.serif": [args.font, "Times New Roman", "Liberation Serif", "DejaVu Serif"],
        "font.size": args.font_size,
        "mathtext.fontset": "stix",  # курсивное N в стиле Times
        "axes.labelsize": args.font_size,
        "xtick.labelsize": args.font_size - 1,
        "ytick.labelsize": args.font_size - 1,
        "axes.labelpad": 2,
    })

    main_data = collect(read_rows(args.csv))
    time_data = collect(read_rows(args.time)) if args.time else main_data
    ns = sorted({n for (_, _), per_n in main_data.items() for n in per_n})

    fig, axes = plt.subplots(1, 3, figsize=(args.width_cm * CM, args.height_cm * CM), dpi=args.dpi)
    memory_reps = ["dense", "block", "svo", "svdag"]
    for scene, (_, linestyle) in SCENES.items():
        for rep in memory_reps:
            points = [(n, m / MIB, lo / MIB, hi / MIB, c) for n, m, lo, hi, c in
                      series(main_data, scene, rep, "memory_bytes", ok_only=False)]
            plot_line(axes[0], points, rep, linestyle)
        for rep in ["dense", "block", "svo"]:
            plot_line(axes[1], series(main_data, scene, rep, "cpu_mean_steps", ok_only=True), rep, linestyle)

    scene = args.time_scene
    for rep in ["dense", "dense-brick"]:
        plot_line(axes[2], series(main_data, scene, rep, "gpu_ms_mean", ok_only=True), rep, "-", ranges=True)
    for rep in ["block", "svo", "svdag"]:
        plot_line(axes[2], series(time_data, scene, rep, "gpu_ms_mean", ok_only=True), rep, "-", ranges=True)

    setup_axis(axes[0], "Объём, МиБ", ns)
    setup_axis(axes[1], "Шагов на луч", ns)
    setup_axis(axes[2], "Время кадра, мс", ns)
    for ax, letter in zip(axes, "абв"):
        ax.set_title(f"{letter})", loc="left", fontsize=args.font_size, pad=3)

    handles = [Line2D([], [], color=STYLE[r]["color"], marker=STYLE[r]["marker"], markersize=3.6, linewidth=1.2,
                      label=STYLE[r]["label"]) for r in STYLE]
    handles.append(Line2D([], [], linestyle="none", label=" "))  # пустая ячейка: сцены – в отдельном столбце
    handles += [Line2D([], [], color="#5f5e5a", linestyle=ls, linewidth=1.0, label=name)
                for name, ls in SCENES.values()]
    fig.legend(handles=handles, loc="lower center", ncol=4, frameon=False, fontsize=args.font_size - 0.5,
               handlelength=2.2, columnspacing=1.2, borderaxespad=0.1)
    fig.tight_layout(rect=(0, 0.15, 1, 1), w_pad=1.2)

    kwargs = {"pil_kwargs": {"compression": "tiff_lzw"}} if args.out.suffix.lower() in (".tif", ".tiff") else {}
    fig.savefig(args.out, dpi=args.dpi, **kwargs)
    if args.preview:
        fig.savefig(args.out.with_suffix(".png"), dpi=args.dpi)
    print(f"{args.out}: {args.width_cm:.1f} x {args.height_cm:.1f} см, {args.dpi} dpi")
    return 0


if __name__ == "__main__":
    sys.exit(main())
