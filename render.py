#!/usr/bin/env python3
"""Render magnetic-pendulum position snapshots (.xy float32 pairs) to PNG.

Coloring: cyclic colormap (default: twilight) applied to the angle of the
particle's *current* position, matching the reference images:
white points up, blue left, dark down, orange/red right at t=0.
"""
import argparse
import glob
import os
import sys

import numpy as np
import matplotlib.pyplot as plt
from PIL import Image


def render_file(path, W, H, cmap_name="twilight", offset=0.25, direction=1,
                supersample=1, out=None, radial=0.0):
    n = W * H
    data = np.fromfile(path, dtype=np.float32)
    assert data.size == 2 * n, f"{path}: expected {2*n} floats, got {data.size}"
    x = data[0::2].reshape(H, W)
    y = data[1::2].reshape(H, W)

    theta = np.arctan2(y, x)  # [-pi, pi]
    # map so that theta=pi/2 (up) -> u=0 (twilight: light), increasing
    # counter-clockwise; offset tunes which angle is white.
    u = (direction * theta / (2 * np.pi) + offset) % 1.0

    cmap = plt.get_cmap(cmap_name)
    rgb = cmap(u)[..., :3]

    if radial > 0:
        r = np.sqrt(x * x + y * y)
        shade = 1.0 / (1.0 + radial * r)
        rgb = rgb * shade[..., None]

    img = (np.clip(rgb, 0, 1) * 255).astype(np.uint8)
    if supersample > 1:
        im = Image.fromarray(img).resize(
            (W // supersample, H // supersample), Image.LANCZOS)
    else:
        im = Image.fromarray(img)
    if out is None:
        out = os.path.splitext(path)[0] + ".png"
    im.save(out)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+", help=".xy files or glob")
    ap.add_argument("-W", type=int, required=True)
    ap.add_argument("-H", dest="hh", type=int, required=True)
    ap.add_argument("--cmap", default="twilight")
    ap.add_argument("--offset", type=float, default=0.25,
                    help="colormap phase offset (0.25 puts cmap-0 at +y)")
    ap.add_argument("--direction", type=int, default=1, choices=[1, -1])
    ap.add_argument("--supersample", type=int, default=1,
                    help="downsample factor (input rendered at Nx)")
    ap.add_argument("--radial", type=float, default=0.0,
                    help="darken with radius: 1/(1+radial*r)")
    ap.add_argument("--outdir", default=None)
    args = ap.parse_args()

    files = []
    for f in args.files:
        files.extend(sorted(glob.glob(f)) if any(c in f for c in "*?[") else [f])
    for f in files:
        out = None
        if args.outdir:
            os.makedirs(args.outdir, exist_ok=True)
            out = os.path.join(
                args.outdir,
                os.path.splitext(os.path.basename(f))[0] + ".png")
        out = render_file(f, args.W, args.hh, args.cmap, args.offset,
                          args.direction, args.supersample, out, args.radial)
        print(out)


if __name__ == "__main__":
    main()
