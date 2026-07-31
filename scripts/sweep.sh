#!/bin/bash
# Parameter sweep at low resolution. Usage: scripts/sweep.sh
set -e
cd "$(dirname "$0")/.."
W=480; H=270
TIMES="2 4 6 8 12 16 24 32"
for vrot in 0.3 0.5 0.7; do
  for fric in 0.1 0.2; do
    for str in 1 3; do
      tag="s${str}_f${fric}_v${vrot}"
      SPRING=0 STRENGTH=$str FRICTION=$fric VROT=$vrot \
        ./sim/pendulum $W $H -5.3333 5.3333 -3 3 0.01 "out/sw_${tag}" $TIMES 2>/dev/null
      echo "$tag done"
    done
  done
done
python3 render.py 'out/sw_*.xy' -W $W -H $H --offset -0.25 > /dev/null
echo rendered
