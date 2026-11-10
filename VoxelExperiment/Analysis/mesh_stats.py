#!/usr/bin/env python3
"""Независимая проверка оценок (4) и (14) на полигональной модели.

Площадь S и ориентационные множители считаются по самой сетке (OBJ), шаг решётки h —
по заголовку binvox (cuda_voxelizer записывает сторону куба вокселизации в строке scale),
число граничных вокселей и объёмы структур — по CSV эксперимента. Тогда:
  * c_pred  = среднее по площади max(|n_x|, |n_y|, |n_z|)       — предсказание (4) для 6-оболочки;
  * c'_pred = среднее по площади (|n_x| + |n_y| + |n_z|)        — для консервативной вокселизации;
  * c_meas  = |dOmega+| / (S / h^2)                              — измерение;
  * A_T = S / (T h^2) — средняя площадь треугольника; порог (14): A_T* = m_tri / (c mu_R).
Только стандартная библиотека Python 3.8+.

    python mesh_stats.py suzanne.obj suzanne_512.binvox suzanne_1024.binvox --csv run.csv
"""

from __future__ import annotations

import argparse
import csv
import math
import sys
from pathlib import Path

M_TRI = {"без нормалей и UV": 18.0, "с нормалями и UV": 28.0}


def load_obj(path: Path) -> tuple[int, int, float, float, float, int]:
    """Число вершин, треугольников, площадь, c_pred, c'_pred, вырожденные треугольники."""
    positions: list[tuple[float, float, float]] = []
    triangles: list[tuple[int, int, int]] = []
    with path.open(encoding="utf-8", errors="replace") as file:
        for line in file:
            parts = line.split()
            if not parts:
                continue
            if parts[0] == "v":
                positions.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif parts[0] == "f":
                ids = []
                for token in parts[1:]:
                    index = int(token.split("/")[0])
                    ids.append(index - 1 if index > 0 else len(positions) + index)
                for i in range(1, len(ids) - 1):  # веер для многоугольников
                    triangles.append((ids[0], ids[i], ids[i + 1]))
    area = 0.0
    weighted_max = 0.0
    weighted_sum = 0.0
    degenerate = 0
    for a, b, c in triangles:
        ax, ay, az = positions[a]
        u = (positions[b][0] - ax, positions[b][1] - ay, positions[b][2] - az)
        v = (positions[c][0] - ax, positions[c][1] - ay, positions[c][2] - az)
        n = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
        length = math.sqrt(n[0] ** 2 + n[1] ** 2 + n[2] ** 2)
        if length == 0.0:
            degenerate += 1
            continue
        tri_area = 0.5 * length
        area += tri_area
        weighted_max += tri_area * max(abs(x) for x in n) / length
        weighted_sum += tri_area * sum(abs(x) for x in n) / length
    return len(positions), len(triangles), area, weighted_max / area, weighted_sum / area, degenerate


def load_binvox_header(path: Path) -> tuple[int, float]:
    """Наибольшее измерение решётки и сторона куба вокселизации (scale) в единицах модели."""
    dims = None
    scale = None
    with path.open("rb") as file:
        for raw in file:
            line = raw.decode("ascii", errors="replace").strip()
            if line.startswith("dim"):
                dims = [int(v) for v in line.split()[1:4]]
            elif line.startswith("scale"):
                scale = float(line.split()[1])
            elif line.startswith("data"):
                break
    if dims is None or scale is None:
        raise ValueError(f"{path}: в заголовке binvox нет строк dim и scale")
    return max(dims), scale


def load_csv(path: Path, scene: str) -> dict[str, tuple[int, int]]:
    """Для сцены model:<имя>: представление -> (граничные воксели, байты)."""
    result: dict[str, tuple[int, int]] = {}
    with path.open(newline="", encoding="utf-8-sig") as file:
        for row in csv.DictReader(file, delimiter=";"):
            if row["scene"] == scene:
                result.setdefault(row["representation"], (int(row["boundary_voxels"]), int(row["memory_bytes"])))
    return result


def fmt(value: float, digits: int = 3) -> str:
    return f"{value:.{digits}f}".replace(".", ",")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("obj", type=Path)
    parser.add_argument("binvox", type=Path, nargs="+", help="вокселизации той же модели при разных N")
    parser.add_argument("--csv", type=Path, action="append", default=[], help="CSV эксперимента (можно повторять)")
    parser.add_argument("--scale", type=float, help="сторона куба вокселизации, если в binvox нет строки scale")
    args = parser.parse_args()
    sys.stdout.reconfigure(encoding="utf-8")

    vertices, triangles, area, c_pred, c_cons, degenerate = load_obj(args.obj)
    print(f"# {args.obj.name}\n")
    print(f"- вершин {vertices}, треугольников {triangles} (вырожденных {degenerate}), площадь {fmt(area, 5)}")
    print(f"- c_pred = {fmt(c_pred)} (6-оболочка, формула (4)); c'_pred = {fmt(c_cons)} (консервативная); "
          f"c'/c = {fmt(c_cons / c_pred)}\n")

    print("| binvox | N | h | S/h² | Ā_T, h² | граничных вокселей | c_meas | c_meas/c_pred; c_meas/c'_pred | SVO, Б/воксель | SVDAG, Б/воксель | "
          "Ā_T* SVO (18 / 28 Б) | Ā_T* SVDAG (18 / 28 Б) | M_SVDAG / M_tri (18 Б) |")
    print("|---|---|---|---|---|---|---|---|---|---|---|---|---|")
    for binvox in args.binvox:
        n, scale = load_binvox_header(binvox)
        if args.scale:
            scale = args.scale
        h = scale / n
        s_h = area / (h * h)
        a_t = s_h / triangles
        cells = [binvox.name, str(n), f"{h:.5g}", f"{s_h:.0f}", fmt(a_t, 1)]
        measured: dict[str, tuple[int, int]] = {}
        for path in args.csv:
            measured.update(load_csv(path, f"model:{binvox.name}"))
        if not measured:
            print("| " + " | ".join(cells + ["–"] * 8) + " |")
            continue
        boundary = next(iter(measured.values()))[0]
        c_meas = boundary / s_h
        cells += [str(boundary), fmt(c_meas), f"{fmt(c_meas / c_pred)}; {fmt(c_meas / c_cons)}"]
        thresholds = []
        mu = {}
        for rep in ("svo", "svdag"):
            if rep in measured:
                mu[rep] = measured[rep][1] / boundary
                cells.append(fmt(mu[rep], 2))
            else:
                cells.append("–")
        for rep in ("svo", "svdag"):
            if rep in mu:
                thresholds.append(" / ".join(fmt(m / (c_meas * mu[rep]), 1) for m in M_TRI.values()))
            else:
                thresholds.append("–")
        cells += thresholds
        cells.append(fmt(measured["svdag"][1] / (triangles * M_TRI["без нормалей и UV"]), 1) if "svdag" in measured else "–")
        print("| " + " | ".join(cells) + " |")
    print("\nТреугольная сетка компактнее представления R, если Ā_T > Ā_T* (критерий (14)).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
