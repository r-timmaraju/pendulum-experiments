#!/usr/bin/env python3
"""Simulate + render + encode the animation.

Simulates at 2560x1440 and downsamples each frame to 1280x720 (2x
supersampling). Runs in chunks so the raw .xy spool stays bounded.
"""
import glob
import os
import subprocess

import numpy as np
import matplotlib.pyplot as plt
import imageio.v2 as imageio
from PIL import Image

SW, SH = 2560, 1440     # simulation grid
OW, OH = 1280, 720      # output frame
T_END = 24.0
DT_FRAME = 0.05
FPS = 30
OFFSET = 0.0            # velocity-angle coloring
CHUNK = 60              # frames per simulation pass (bounds .xy spool size)
ENV = dict(os.environ, DUMPV="1", SPRING="0", STRENGTH="2", FRICTION="0.02",
           VKEP="-0.9", BGA="0.15", BGD="1", HEIGHT="0.45", MAGRADIUS="2")

os.makedirs("out/anim", exist_ok=True)
cmap = plt.get_cmap("twilight")
writer = imageio.get_writer("renders/pendulum_swirl.mp4", fps=FPS,
                            codec="libx264", quality=8,
                            ffmpeg_params=["-pix_fmt", "yuv420p"])

all_times = [round(t, 3) for t in np.arange(DT_FRAME, T_END + 1e-9, DT_FRAME)]
for c0 in range(0, len(all_times), CHUNK):
    times = all_times[c0:c0 + CHUNK]
    args = ["./sim/pendulum", str(SW), str(SH), "-10.6667", "10.6667",
            "-6", "6", "0.01", "out/anim/f"] + [str(t) for t in times]
    print(f"chunk t={times[0]}..{times[-1]}: simulating {len(times)} frames")
    subprocess.run(args, env=ENV, check=True,
                   stderr=subprocess.DEVNULL)
    for f in sorted(glob.glob("out/anim/f_t*.xy")):
        d = np.fromfile(f, dtype=np.float32)
        vx = d[2::4].reshape(SH, SW); vy = d[3::4].reshape(SH, SW)
        u = (np.arctan2(vy, vx) / (2 * np.pi) + OFFSET) % 1.0
        rgb = (cmap(u)[..., :3] * 255).astype(np.uint8)
        frame = Image.fromarray(rgb).resize((OW, OH), Image.LANCZOS)
        writer.append_data(np.asarray(frame))
        os.remove(f)
writer.close()
print("renders/pendulum_swirl.mp4 done")
