#!/usr/bin/env python3
"""Обработка CSV вычислительного эксперимента (ExperimentRunner) в таблицы для статьи.

Только стандартная библиотека Python 3.8+. Примеры:
    python analyze.py voxel_experiment.csv
    python analyze.py run1.csv run2.csv --out tables.md
    python analyze.py run.csv --fit-min-n 512      # показатели роста только по N >= 512

Что выводится (Markdown):
  1. объём структур, МиБ, и отношение SVO / SVDAG (табл. 2 статьи);
  2. показатели роста alpha (M ~ N^alpha) с 95 % доверительным интервалом
     (МНК в логарифмических координатах, t-распределение с n - 2 степенями свободы);
  3. локальные наклоны отношения Q = M_SVO / M_SVDAG между соседними N и экстраполяция Q
     до N = 2^17 двумя способами: по МНК-наклону и по последнему локальному наклону;
  4. шаги на луч / время кадра, мс (табл. 3 статьи), с разбросом по ракурсам, если он > 50 %;
  5. чтения буфера на луч и время для спуска от корня и со стеком (SVO, SVDAG);
  6. зависимость времени плотной сетки от ракурса (линейная раскладка и кирпичи);
  7. удельный объём SVDAG, бит на граничный воксель.

Файлы старого формата (без столбцов traversal и cpu_mean_fetches) читаются; SVO и SVDAG
в них считаются измеренными со спуском от корня (--legacy-traversal).
"""

from __future__ import annotations

import argparse
import csv
import math
import re
import statistics
import sys
from collections import defaultdict
from pathlib import Path

MIB = 1024.0 * 1024.0
REPRESENTATION_ORDER = ["dense", "dense-brick", "block", "svo", "svdag"]
HIERARCHICAL = {"svo", "svdag"}
# Квантили t-распределения уровня 0,975 для 1..30 степеней свободы.
T975 = [12.706, 4.303, 3.182, 2.776, 2.571, 2.447, 2.365, 2.306, 2.262, 2.228, 2.201, 2.179, 2.160, 2.145, 2.131,
        2.120, 2.110, 2.101, 2.093, 2.086, 2.080, 2.074, 2.069, 2.064, 2.060, 2.056, 2.052, 2.048, 2.045, 2.042]


def t975(df: int) -> float:
    return T975[df - 1] if 1 <= df <= len(T975) else 1.96


def fmt(value: float | None, digits: int = 2) -> str:
    if value is None or (isinstance(value, float) and math.isnan(value)):
        return "–"
    text = f"{value:.{digits}f}"
    return text.replace(".", ",")


def fmt_int(value: int) -> str:
    return f"{value:,}".replace(",", " ")


def power_fit(xs: list[float], ys: list[float]) -> tuple[float, float | None]:
    """Показатель alpha в y = C x^alpha и полуширина 95 % интервала (None, если точек < 3)."""
    lx = [math.log(x) for x in xs]
    ly = [math.log(y) for y in ys]
    n = len(lx)
    mx, my = statistics.fmean(lx), statistics.fmean(ly)
    sxx = sum((a - mx) ** 2 for a in lx)
    if n < 2 or sxx == 0.0:
        return float("nan"), None
    slope = sum((a - mx) * (b - my) for a, b in zip(lx, ly)) / sxx
    if n < 3:
        return slope, None
    intercept = my - slope * mx
    rss = sum((b - intercept - slope * a) ** 2 for a, b in zip(lx, ly))
    se = math.sqrt(rss / (n - 2) / sxx)
    return slope, t975(n - 2) * se


def fmt_alpha(fit: tuple[float, float | None]) -> str:
    slope, half = fit
    if half is None:
        return fmt(slope)
    return f"{fmt(slope)} ± {fmt(half)}"


class Rows:
    time_column = "gpu_ms_mean"

    def __init__(self, paths: list[Path], legacy_traversal: str) -> None:
        self.rows: list[dict] = []
        for path in paths:
            with path.open(newline="", encoding="utf-8-sig") as file:
                for raw in csv.DictReader(file, delimiter=";"):
                    self.rows.append(self._normalize(raw, legacy_traversal))

    @staticmethod
    def _normalize(raw: dict, legacy_traversal: str) -> dict:
        rep = raw["representation"]
        traversal = raw.get("traversal")
        if traversal is None:
            traversal = legacy_traversal if rep in HIERARCHICAL else "-"

        def number(key: str, default: float = float("nan")) -> float:
            value = raw.get(key)
            return float(value) if value not in (None, "") else default

        steps = number("cpu_mean_steps")
        fetches = number("cpu_mean_fetches")
        if raw["gpu_status"].startswith("skipped: dense grid"):  # структура не строилась
            steps = fetches = float("nan")
        scene = raw["scene"]
        # model:suzanne_512.binvox при N = 512 -> model:suzanne, чтобы разные N одной модели шли вместе
        match = re.fullmatch(r"(model:.+?)[_-]?(\d+)\.binvox", scene)
        if match and int(match.group(2)) == int(raw["N"]):
            scene = match.group(1)
        return {
            "scene": scene,
            "N": int(raw["N"]),
            "rep": rep,
            "traversal": traversal,
            "view": int(raw["view"]),
            "boundary": int(raw["boundary_voxels"]),
            "build_ms": number("build_ms"),
            "memory": int(raw["memory_bytes"]),
            "steps": steps,
            "fetches": fetches,
            "status": raw["gpu_status"],
            "ms": number(Rows.time_column if raw.get(Rows.time_column) not in (None, "") else "gpu_ms_mean")
            if raw["gpu_status"] == "ok" else float("nan"),
            "ms_mean": number("gpu_ms_mean") if raw["gpu_status"] == "ok" else float("nan"),
            "ms_std": number("gpu_ms_std") if raw["gpu_status"] == "ok" else float("nan"),
        }

    def scenes(self) -> list[str]:
        order: list[str] = []
        for row in self.rows:
            if row["scene"] not in order:
                order.append(row["scene"])
        return order

    def resolutions(self, scene: str) -> list[int]:
        return sorted({r["N"] for r in self.rows if r["scene"] == scene})

    def representations(self) -> list[str]:
        present = {r["rep"] for r in self.rows}
        return [r for r in REPRESENTATION_ORDER if r in present] + sorted(present - set(REPRESENTATION_ORDER))

    def variants(self) -> list[tuple[str, str]]:
        present = {(r["rep"], r["traversal"]) for r in self.rows}
        order = []
        for rep in self.representations():
            for traversal in ("-", "restart", "stack"):
                if (rep, traversal) in present:
                    order.append((rep, traversal))
        return order

    def select(self, scene: str, n: int, rep: str, traversal: str | None = None) -> list[dict]:
        return [r for r in self.rows if r["scene"] == scene and r["N"] == n and r["rep"] == rep and
                (traversal is None or r["traversal"] == traversal)]

    def memory(self, scene: str, n: int, rep: str) -> tuple[int, bool] | None:
        """Объём и признак «вычислен, а не построен» (плотная сетка при больших N)."""
        rows = self.select(scene, n, rep)
        if not rows:
            return None
        return rows[0]["memory"], rows[0]["status"].startswith("skipped: dense grid")

    def boundary(self, scene: str, n: int) -> int | None:
        rows = [r for r in self.rows if r["scene"] == scene and r["N"] == n]
        return rows[0]["boundary"] if rows else None


def variant_name(rep: str, traversal: str) -> str:
    return rep if traversal == "-" else f"{rep} ({traversal})"


def memory_section(data: Rows, fit_min_n: int, out: list[str]) -> None:
    reps = data.representations()
    out.append("## Объём структур данных, МиБ\n")
    header = ["Сцена", "N", "Граничных вокселей"] + reps
    has_ratio = "svo" in reps and "svdag" in reps
    if has_ratio:
        header.append("SVO / SVDAG")
    out.append("| " + " | ".join(header) + " |")
    out.append("|" + "---|" * len(header))
    for scene in data.scenes():
        for n in data.resolutions(scene):
            cells = [scene, str(n), fmt_int(data.boundary(scene, n) or 0)]
            for rep in reps:
                value = data.memory(scene, n, rep)
                if value is None:
                    cells.append("–")
                else:
                    cells.append(fmt(value[0] / MIB, 3 if value[0] < 10 * MIB else 2) + ("*" if value[1] else ""))
            if has_ratio:
                svo, dag = data.memory(scene, n, "svo"), data.memory(scene, n, "svdag")
                cells.append(fmt(svo[0] / dag[0]) if svo and dag else "–")
            out.append("| " + " | ".join(cells) + " |")
    out.append("\n\\* объём вычислен по N³/8, структура не строилась.\n")

    out.append(f"### Показатели роста α (M ∝ N^α), 95 % интервал, N ≥ {fit_min_n}\n")
    header = ["Сцена", "точек", "граничные воксели"] + reps + (["SVO / SVDAG"] if has_ratio else [])
    out.append("| " + " | ".join(header) + " |")
    out.append("|" + "---|" * len(header))
    for scene in data.scenes():
        ns = [n for n in data.resolutions(scene) if n >= fit_min_n]
        if len(ns) < 2:
            continue
        cells = [scene, str(len(ns)), fmt_alpha(power_fit(ns, [data.boundary(scene, n) for n in ns]))]
        for rep in reps:
            pairs = [(n, data.memory(scene, n, rep)) for n in ns]
            pairs = [(n, m[0]) for n, m in pairs if m and m[0] > 0]
            cells.append(fmt_alpha(power_fit([p[0] for p in pairs], [p[1] for p in pairs])) if len(pairs) >= 2 else "–")
        if has_ratio:
            q = [(n, data.memory(scene, n, "svo"), data.memory(scene, n, "svdag")) for n in ns]
            q = [(n, a[0] / b[0]) for n, a, b in q if a and b]
            cells.append(fmt_alpha(power_fit([p[0] for p in q], [p[1] for p in q])) if len(q) >= 2 else "–")
        out.append("| " + " | ".join(cells) + " |")
    out.append("\nПри трёх точках у МНК одна степень свободы (t = 12,7), поэтому интервал широкий.\n")

    if not has_ratio:
        return
    out.append("### Отношение Q = M_SVO / M_SVDAG: локальные наклоны и экстраполяция до N = 2^17\n")
    out.append("| Сцена | Локальные наклоны d ln Q / d ln N | Q при N_max | Q(2^17) по МНК | Q(2^17) по последнему наклону |")
    out.append("|---|---|---|---|---|")
    target = 2 ** 17
    for scene in data.scenes():
        q = [(n, data.memory(scene, n, "svo"), data.memory(scene, n, "svdag")) for n in data.resolutions(scene)]
        q = [(n, a[0] / b[0]) for n, a, b in q if a and b]
        if len(q) < 2:
            continue
        local = [(q[i][0], q[i + 1][0], math.log(q[i + 1][1] / q[i][1]) / math.log(q[i + 1][0] / q[i][0]))
                 for i in range(len(q) - 1)]
        n_max, q_max = q[-1]
        fit_points = [p for p in q if p[0] >= fit_min_n]
        slope, half = power_fit([p[0] for p in fit_points], [p[1] for p in fit_points])
        ls_value = q_max * (target / n_max) ** slope
        ls_text = fmt(ls_value, 0)
        if half is not None:
            ls_text += f" ({fmt(q_max * (target / n_max) ** (slope - half), 0)}…{fmt(q_max * (target / n_max) ** (slope + half), 0)})"
        last_value = q_max * (target / n_max) ** local[-1][2]
        out.append(f"| {scene} | " + "; ".join(f"{a}→{b}: {fmt(s)}" for a, b, s in local) +
                   f" | {fmt(q_max)} | {ls_text} | {fmt(last_value, 0)} |")
    out.append("\nВ скобках — диапазон по 95 % интервалу наклона. Для сравнения с [10] отношение числа узлов "
               "R_n = Q · 4(1 + k̄)/s, формула (12).\n")

    out.append("### SVDAG: бит на граничный воксель\n")
    out.append("| Сцена | " + " | ".join(f"N = {n}" for n in sorted({r['N'] for r in data.rows})) + " |")
    out.append("|---|" + "---|" * len({r["N"] for r in data.rows}))
    for scene in data.scenes():
        cells = [scene]
        for n in sorted({r["N"] for r in data.rows}):
            dag = data.memory(scene, n, "svdag")
            boundary = data.boundary(scene, n)
            cells.append(fmt(8.0 * dag[0] / boundary) if dag and boundary else "–")
        out.append("| " + " | ".join(cells) + " |")
    out.append("")


def view_stats(rows: list[dict]) -> tuple[float, float, float, float]:
    """Средние по ракурсам шаги и время, отношение max/min времени и шагов."""
    steps = [r["steps"] for r in rows if not math.isnan(r["steps"])]
    times = [r["ms"] for r in rows if not math.isnan(r["ms"])]
    mean_steps = statistics.fmean(steps) if steps else float("nan")
    mean_ms = statistics.fmean(times) if times else float("nan")
    time_spread = max(times) / min(times) if len(times) > 1 and min(times) > 0 else float("nan")
    step_spread = max(steps) / min(steps) if len(steps) > 1 and min(steps) > 0 else float("nan")
    return mean_steps, mean_ms, time_spread, step_spread


def steps_section(data: Rows, fit_min_n: int, out: list[str]) -> None:
    variants = data.variants()
    out.append("## Среднее число шагов на луч / время кадра, мс\n")
    out.append("Среднее по ракурсам; в скобках — диапазон времени по ракурсам, если разброс больше 50 %.\n")
    header = ["Сцена", "N"] + [variant_name(*v) for v in variants]
    out.append("| " + " | ".join(header) + " |")
    out.append("|" + "---|" * len(header))
    for scene in data.scenes():
        for n in data.resolutions(scene):
            cells = [scene, str(n)]
            for rep, traversal in variants:
                rows = data.select(scene, n, rep, traversal)
                if not rows:
                    cells.append("–")
                    continue
                steps, ms, spread, _ = view_stats(rows)
                text = f"{fmt(steps)} / {fmt(ms)}"
                if not math.isnan(spread) and spread > 1.5:
                    times = [r["ms"] for r in rows if not math.isnan(r["ms"])]
                    text += f" ({fmt(min(times), 1)}–{fmt(max(times), 1)})"
                cells.append(text)
            out.append("| " + " | ".join(cells) + " |")

    out.append(f"\n### Показатели роста числа шагов и времени, N ≥ {fit_min_n}\n")
    out.append("| Сцена | Вариант | α шагов | α времени |")
    out.append("|---|---|---|---|")
    for scene in data.scenes():
        for rep, traversal in variants:
            points = []
            for n in data.resolutions(scene):
                if n < fit_min_n:
                    continue
                rows = data.select(scene, n, rep, traversal)
                if rows:
                    steps, ms, _, _ = view_stats(rows)
                    points.append((n, steps, ms))
            steps_pts = [(n, s) for n, s, _ in points if s > 0 and not math.isnan(s)]
            time_pts = [(n, t) for n, _, t in points if t > 0 and not math.isnan(t)]
            if len(steps_pts) < 2:
                continue
            a_steps = fmt_alpha(power_fit([p[0] for p in steps_pts], [p[1] for p in steps_pts]))
            a_time = fmt_alpha(power_fit([p[0] for p in time_pts], [p[1] for p in time_pts])) if len(time_pts) >= 2 else "–"
            out.append(f"| {scene} | {variant_name(rep, traversal)} | {a_steps} | {a_time} |")
    out.append("")


def traversal_section(data: Rows, out: list[str]) -> None:
    pairs = [(rep, data) for rep in ("svo", "svdag")]
    if not any(data.select(s, n, rep, "stack") and data.select(s, n, rep, "restart")
               for rep, _ in pairs for s in data.scenes() for n in data.resolutions(s)):
        return
    out.append("## Спуск от корня и со стеком (SVO, SVDAG)\n")
    out.append("| Сцена | N | Представление | Чтений на луч: от корня / стек | Время, мс: от корня / стек | Ускорение |")
    out.append("|---|---|---|---|---|---|")
    for scene in data.scenes():
        for n in data.resolutions(scene):
            for rep in ("svo", "svdag"):
                restart = data.select(scene, n, rep, "restart")
                stack = data.select(scene, n, rep, "stack")
                if not restart or not stack:
                    continue
                fr = statistics.fmean(r["fetches"] for r in restart)
                fs = statistics.fmean(r["fetches"] for r in stack)
                _, tr, _, _ = view_stats(restart)
                _, ts, _, _ = view_stats(stack)
                speedup = tr / ts if ts and not math.isnan(ts) and not math.isnan(tr) else float("nan")
                out.append(f"| {scene} | {n} | {rep} | {fmt(fr, 1)} / {fmt(fs, 1)} | {fmt(tr)} / {fmt(ts)} | {fmt(speedup)} |")
    out.append("")


def dense_section(data: Rows, out: list[str]) -> None:
    reps = [r for r in ("dense", "dense-brick") if r in data.representations()]
    if not reps:
        return
    out.append("## Плотная сетка: зависимость от ракурса\n")
    out.append("| Сцена | N | Раскладка | Время по ракурсам, мс | max/min времени | max/min шагов |")
    out.append("|---|---|---|---|---|---|")
    for scene in data.scenes():
        for n in data.resolutions(scene):
            for rep in reps:
                rows = sorted(data.select(scene, n, rep), key=lambda r: r["view"])
                times = [r["ms"] for r in rows if not math.isnan(r["ms"])]
                if not times:
                    continue
                _, _, time_spread, step_spread = view_stats(rows)
                out.append(f"| {scene} | {n} | {'линейная' if rep == 'dense' else 'кирпичи 8³'} | " +
                           "; ".join(fmt(t) for t in times) + f" | {fmt(time_spread)} | {fmt(step_spread)} |")
    out.append("")


def noise_section(data: Rows, out: list[str]) -> None:
    """Замеры с разбросом по сериям больше 5 %: признак помех, время ненадёжно."""
    noisy = defaultdict(list)
    total = 0
    for r in data.rows:
        if math.isnan(r["ms_mean"]) or r["ms_mean"] <= 0:
            continue
        total += 1
        if r["ms_std"] / r["ms_mean"] > 0.05:
            noisy[(r["scene"], r["N"])].append(r["ms_std"] / r["ms_mean"])
    if not noisy:
        return
    count = sum(len(v) for v in noisy.values())
    out.append("## Замеры с разбросом по сериям больше 5 %\n")
    out.append(f"Всего {count} из {total}. Время в этих строках ненадёжно (помехи от других процессов "
               "или смена частоты GPU).\n")
    out.append("| Сцена | N | Замеров | Наибольший разброс, % |")
    out.append("|---|---|---|---|")
    for (scene, n), values in noisy.items():
        out.append(f"| {scene} | {n} | {len(values)} | {fmt(100 * max(values), 0)} |")
    out.append("")


def status_section(data: Rows, out: list[str]) -> None:
    problems = sorted({(r["scene"], r["N"], variant_name(r["rep"], r["traversal"]), r["status"])
                       for r in data.rows if r["status"] != "ok"})
    if problems:
        out.append("## Строки без замера на GPU\n")
        for scene, n, variant, status in problems:
            out.append(f"- {scene}, N = {n}, {variant}: {status}")
        out.append("")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("csv", nargs="+", type=Path, help="файлы CSV эксперимента")
    parser.add_argument("--out", type=Path, help="записать Markdown в файл (по умолчанию — в stdout)")
    parser.add_argument("--fit-min-n", type=int, default=0, help="показатели роста только по N >= этого значения")
    parser.add_argument("--legacy-traversal", default="restart", choices=["restart", "stack"],
                        help="способ спуска SVO/SVDAG для CSV старого формата (по умолчанию restart)")
    parser.add_argument("--time", default="mean", choices=["mean", "min"],
                        help="время кадра: среднее по сериям или самый быстрый кадр (столбец gpu_ms_min)")
    args = parser.parse_args()
    Rows.time_column = "gpu_ms_mean" if args.time == "mean" else "gpu_ms_min"

    data = Rows(args.csv, args.legacy_traversal)
    out: list[str] = ["# Результаты эксперимента\n"]
    memory_section(data, args.fit_min_n, out)
    steps_section(data, args.fit_min_n, out)
    traversal_section(data, out)
    dense_section(data, out)
    noise_section(data, out)
    status_section(data, out)
    text = "\n".join(out) + "\n"
    if args.out:
        args.out.write_text(text, encoding="utf-8")
    else:
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
