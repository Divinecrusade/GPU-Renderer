#!/usr/bin/env python3
"""Сборка рисунка 2 статьи из снимков окна просмотра (клавиша P).

Четыре панели: а) затенение (SVO); б)–г) тепловые карты числа шагов на луч для плотной
сетки, блочной сетки и SVO. Справа – шкала цветов той же палитры, что в VoxelTracePS.hlsl:
h = log2(1 + шаги) / log2(2 + 3N), цвет = (h, h^2, 1 - h).

Нужны matplotlib и Pillow:  pip install matplotlib pillow

Примеры:
    # снимки лежат в текущем каталоге, ракурс эксперимента 0 (клавиша 1)
    python make_figure.py --scene terrain --resolution 1024 --view 0
    # явные пути, раскладка 2 x 2, обрезка краёв кадра
    python make_figure.py --resolution 1024 --shade a.bmp --dense b.bmp --block c.bmp --svo d.bmp \\
        --layout grid --crop 0.1,0,0.9,1
"""

from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path

CM = 1 / 2.54  # дюймов в сантиметре


def palette(h: float) -> tuple[float, float, float]:
    """Палитра тепловой карты шейдера."""
    h = min(max(h, 0.0), 1.0)
    return h, h * h, 1.0 - h


def steps_to_h(steps: float, n: int) -> float:
    return math.log2(1.0 + steps) / math.log2(2.0 + 3.0 * n)


def load_panels(paths: list[Path], crop: tuple[float, float, float, float]):
    from PIL import Image

    images = []
    for path in paths:
        if not path.is_file():
            sys.exit(f"Нет файла {path}. Снимок делается клавишей P в окне просмотра (--voxel-view-only).")
        image = Image.open(path).convert("RGB")
        w, h = image.size
        box = (round(crop[0] * w), round(crop[1] * h), round(crop[2] * w), round(crop[3] * h))
        images.append(image.crop(box))
    sizes = {im.size for im in images}
    if len(sizes) != 1:
        print(f"Предупреждение: размеры снимков различаются: {sorted(sizes)}", file=sys.stderr)
    return images


def resized(image, width_cm: float, height_cm: float, dpi: int):
    from PIL import Image

    size = (max(1, round(width_cm * CM * dpi)), max(1, round(height_cm * CM * dpi)))
    return image.resize(size, Image.LANCZOS)


def build_figure(images, n: int, layout: str, width_cm: float, dpi: int, font: str, font_size: float):
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    import numpy as np

    plt.rcParams.update({
        "font.family": "serif",
        "font.serif": [font, "Times New Roman", "Liberation Serif", "DejaVu Serif"],
        "font.size": font_size,
    })

    aspect = images[0].size[1] / images[0].size[0]  # высота / ширина панели
    gap = 0.15        # промежуток между панелями, см
    label_h = 0.45    # строка подписи «а)» под панелью, см
    bar_w = 0.2       # ширина шкалы, см
    bar_text = 1.15   # подписи делений и название шкалы, см
    right = gap + bar_w + bar_text
    top = 0.15        # поле сверху: верхняя подпись шкалы не обрезается, см

    cols, rows = (4, 1) if layout == "row" else (2, 2)
    panel_w = (width_cm - right - gap * (cols - 1)) / cols
    panel_h = panel_w * aspect
    height_cm = top + rows * (panel_h + label_h) + gap * (rows - 1)

    fig = plt.figure(figsize=(width_cm * CM, height_cm * CM), dpi=dpi)
    labels = ["а)", "б)", "в)", "г)"]
    for i, image in enumerate(images):
        r, c = divmod(i, cols)
        x = c * (panel_w + gap)
        y = height_cm - top - r * (panel_h + label_h + gap) - panel_h
        ax = fig.add_axes([x / width_cm, y / height_cm, panel_w / width_cm, panel_h / height_cm])
        ax.imshow(np.asarray(resized(image, panel_w, panel_h, dpi)), interpolation="none", aspect="auto")
        ax.set_axis_off()
        fig.text((x + panel_w / 2) / width_cm, (y - label_h / 2) / height_cm, labels[i], ha="center", va="center")

    # Шкала цветов – высотой с панель тепловых карт (в раскладке 2 x 2 – на обе строки)
    bar_x = width_cm - right + gap
    bar_top = height_cm - top
    bar_bottom = bar_top - panel_h if layout == "row" else label_h
    bar = fig.add_axes([bar_x / width_cm, bar_bottom / height_cm, bar_w / width_cm, (bar_top - bar_bottom) / height_cm])
    gradient = np.array([palette(h) for h in np.linspace(0.0, 1.0, 512)])[:, None, :]
    bar.imshow(gradient, origin="lower", aspect="auto", extent=(0, 1, 0, 1), interpolation="bilinear")
    bar.set_xticks([])
    worst = 3 * n + 1
    ticks = [s for s in (1, 10, 100, 1000, 10000) if steps_to_h(s, n) < steps_to_h(worst, n) - 0.08] + [worst]
    bar.yaxis.tick_right()
    bar.set_yticks([steps_to_h(s, n) for s in ticks])
    bar.set_yticklabels([f"{s}" for s in ticks], fontsize=font_size - 1.5)
    bar.tick_params(axis="y", length=2, width=0.5, pad=1.5)
    for spine in bar.spines.values():
        spine.set_linewidth(0.5)
    bar.set_ylabel("шагов на луч", fontsize=font_size - 1.5, rotation=90, labelpad=2)
    bar.yaxis.set_label_position("right")
    return fig, width_cm, height_cm


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--resolution", "-N", type=int, required=True, help="разрешение сцены N (для шкалы 1…3N+1)")
    parser.add_argument("--scene", default="terrain", help="сцена в имени файла снимка (по умолчанию terrain)")
    parser.add_argument("--view", default="0", help="ракурс эксперимента 0–2 (клавиши 1–3) или суффикс вроде az123")
    parser.add_argument("--dir", type=Path, default=Path("."), help="каталог со снимками")
    parser.add_argument("--shade", type=Path, help="снимок панели а) (по умолчанию – SVO, затенение)")
    parser.add_argument("--dense", type=Path, help="тепловая карта плотной сетки")
    parser.add_argument("--block", type=Path, help="тепловая карта блочной сетки")
    parser.add_argument("--svo", type=Path, help="тепловая карта SVO")
    parser.add_argument("--layout", choices=["row", "grid"], default="row", help="4 в ряд или 2 x 2")
    parser.add_argument("--width-cm", type=float, help="ширина рисунка, см (по умолчанию 17 для row, 12 для grid)")
    parser.add_argument("--crop", default="0,0,1,1", help="обрезка кадра: левый,верхний,правый,нижний край в долях")
    parser.add_argument("--dpi", type=int, default=300)
    parser.add_argument("--font", default="Times New Roman")
    parser.add_argument("--font-size", type=float, default=10.0, help="кегль подписей, пт")
    parser.add_argument("--out", type=Path, default=Path("figure2.tif"))
    parser.add_argument("--preview", action="store_true", help="дополнительно сохранить PNG для просмотра")
    args = parser.parse_args()

    view = f"view{args.view}" if args.view.isdigit() else args.view
    prefix = f"capture_{args.scene}_N{args.resolution}"
    default = {
        "shade": f"{prefix}_svo_shade_{view}.bmp",
        "dense": f"{prefix}_dense_steps_{view}.bmp",
        "block": f"{prefix}_block_steps_{view}.bmp",
        "svo": f"{prefix}_svo_steps_{view}.bmp",
    }
    paths = [getattr(args, key) or args.dir / name for key, name in default.items()]
    crop = tuple(float(v) for v in args.crop.split(","))
    if len(crop) != 4 or not (0 <= crop[0] < crop[2] <= 1 and 0 <= crop[1] < crop[3] <= 1):
        sys.exit("--crop: нужны четыре доли 0…1: левый < правый, верхний < нижний")

    width_cm = args.width_cm or (17.0 if args.layout == "row" else 12.0)
    images = load_panels(paths, crop)
    fig, w, h = build_figure(images, args.resolution, args.layout, width_cm, args.dpi, args.font, args.font_size)
    fig.savefig(args.out, dpi=args.dpi, pil_kwargs={"compression": "tiff_lzw"} if args.out.suffix.lower() in
                (".tif", ".tiff") else None)
    if args.preview:
        fig.savefig(args.out.with_suffix(".png"), dpi=args.dpi)
    print(f"{args.out}: {w:.1f} x {h:.1f} см, {args.dpi} dpi")
    return 0


if __name__ == "__main__":
    sys.exit(main())
