#!/usr/bin/env python3
"""Simulate + render + encode the animation in chunks (to bound disk use)."""
import glob
import os
import subprocess
import sys

import numpy as np
import matplotlib.pyplot as plt
import imageio.v2 as imageio

W, H = 1280, 720
T_END = 20.0
DT_FRAME = 0.05
FPS = 30
OFFSET = -0.25
ENV = dict(os.environ, SPRING="0", STRENGTH="1", FRICTION="0.2",
           VKEP="-1.0", BGA="1", BGD="1", HEIGHT="0.3")

os.makedirs("out/anim", exist_ok=True)
times = [round(t, 3) for t in np.arange(DT_FRAME, T_END + 1e-9, DT_FRAME)]
args = ["./sim/pendulum", str(W), str(H), "-7.1111", "7.1111", "-4", "4",
        "0.01", "out/anim/f"] + [str(t) for t in times]
print(f"simulating {len(times)} frames ...")
subprocess.run(args, env=ENV, check=True)

cmap = plt.get_cmap("twilight")
writer = imageio.get_writer("renders/pendulum_swirl.mp4", fps=FPS,
                            codec="libx264", quality=8,
                            ffmpeg_params=["-pix_fmt", "yuv420p"])
files = sorted(glob.glob("out/anim/f_t*.xy"))
print(f"encoding {len(files)} frames ...")
for i, f in enumerate(files):
    d = np.fromfile(f, dtype=np.float32)
    x = d[0::2].reshape(H, W); y = d[1::2].reshape(H, W)
    u = (np.arctan2(y, x) / (2 * np.pi) + OFFSET) % 1.0
    rgb = (cmap(u)[..., :3] * 255).astype(np.uint8)
    writer.append_data(rgb)
    os.remove(f)
    if i % 50 == 0:
        print(f"  {i}/{len(files)}")
writer.close()
print("renders/pendulum_swirl.mp4 done")
