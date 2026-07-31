# Experiment log — magnetic pendulum fractal basins

Goal: replicate the r/generative "Fractal basins of attraction" images
(5-magnet pendulum, pixel = initial condition, color = twilight colormap of
the angle of the particle's *current* position).

## Model

    r'' = -friction * r' - spring * r + strength * sum_i (m_i - r) / (|m_i - r|^2 + d^2)^(3/2)

5 magnets on a regular pentagon of radius 1, first magnet pointing up (+y).
Initial condition per pixel: position = pixel coordinate, velocity = vrot * (-y, x)
(rigid rotation field). RK4, fixed dt.

## E1 — classic textbook params (spring=0.5, friction=0.2, v0=0)

Result: mirror-symmetric kaleidoscope, straight rays. WRONG — the reference
has a global differential swirl and no mirror symmetry. Conclusions:
- Reference must have nonzero initial *velocity* per pixel (author: "position
  and velocity") → rigid rotation field v0 = ω×r.
- Harmonic spring gives linear far-field dynamics → straight rays forever.
  The author says "potential with five minima", i.e. NO spring. Then the far
  field is ~5/r² central attraction → Kepler-like differential winding, which
  is exactly the reference's giant swirl.

## E2 — spring=0, friction=0.1, vrot ∈ {0.15, 0.3, 0.5}

Swirl appears; color orientation fixed with cmap offset −0.25 (white up,
blue left, dark down, orange right at t=0 — matches reference frame 1).
But chaos develops too fast/wide: broad salt-and-pepper annulus by t=8.

## E3 — sweep strength ∈ {1,3} × friction ∈ {0.1,0.2} × vrot ∈ {0.3,0.5,0.7}

Contact sheet: out/sweep_sheet.png. Findings:
- strength=3: everything dissolves into noise quickly. strength=1 better.
- friction=0.2 keeps the field coherent far longer (creamy laminar arms with
  chaotic filigree bands) — closest to reference image 3.
- vrot 0.3–0.5 both plausible: r_eq=(5·S/ω²)^(1/3) sets the "active disk"
  radius; ω=0.5, S=1 → r_eq≈2.7 ≈ frame height — good.
- Reference "image 2" (smooth swirl + ring of ~10 pinwheel singularities +
  intact 5-petal flower) ≈ t 4–6 here. "Image 3" (dense marbling, eddies,
  metallic) ≈ t 12–32.

Next: full-res validation (1920×1080) of s=1, f=0.2, v=0.5, extent ±3.
