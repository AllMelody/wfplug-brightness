#!/usr/bin/env python3
"""Render brightness-panel.png at hicolor sizes.

Matches the PiXtrix blue used by the wifi/bluetooth icons (#00A7FF). Drawn at
4× canvas then downsampled with LANCZOS for clean anti-aliased edges at small
sizes.
"""
from pathlib import Path
from PIL import Image, ImageDraw
import math

BLUE = (0x00, 0xA7, 0xFF, 0xFF)
SIZES = [16, 22, 24, 32, 48]
OUT_BASE = Path(__file__).resolve().parent.parent / "data/icons/hicolor"


def draw_sun(size):
    scale = 8
    S = size * scale
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    cx = cy = S / 2
    # Sun body disk (larger than before for better center/ray contrast)
    body_r = S * 0.22
    d.ellipse(
        [cx - body_r, cy - body_r, cx + body_r, cy + body_r],
        fill=BLUE,
    )

    # 8 rays: chunky rounded-ended lines from r1 to r2.
    # r1 pushed outward so rays don't merge with the body into an asterisk.
    r1 = S * 0.33
    r2 = S * 0.44
    ray_w = int(S * 0.09)
    for i in range(8):
        theta = i * math.pi / 4
        x1 = cx + r1 * math.cos(theta)
        y1 = cy + r1 * math.sin(theta)
        x2 = cx + r2 * math.cos(theta)
        y2 = cy + r2 * math.sin(theta)
        d.line([(x1, y1), (x2, y2)], fill=BLUE, width=ray_w)
        # Rounded caps
        d.ellipse(
            [x2 - ray_w / 2, y2 - ray_w / 2, x2 + ray_w / 2, y2 + ray_w / 2],
            fill=BLUE,
        )
        d.ellipse(
            [x1 - ray_w / 2, y1 - ray_w / 2, x1 + ray_w / 2, y1 + ray_w / 2],
            fill=BLUE,
        )

    return img.resize((size, size), Image.LANCZOS)


for size in SIZES:
    out_dir = OUT_BASE / f"{size}x{size}" / "status"
    out_dir.mkdir(parents=True, exist_ok=True)
    out = out_dir / "brightness-panel.png"
    draw_sun(size).save(out)
    print(f"wrote {out.relative_to(OUT_BASE.parent.parent)}")
