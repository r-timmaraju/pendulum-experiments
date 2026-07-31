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

## E4 — strong wells / slow rigid rotation (sub-circular plunge), extent ±4

strength ∈ {5,10,20} × vrot ∈ {0.1,0.2,0.3}: everything decoheres into
salt-and-pepper by t≈8. Plunging radial infall through the pentagon core
destroys coherence. Also tried wells + broad central attractor
(CENTRAL/CD) — same failure. Conclusion: the reference's coherence means
most particles *circulate* on near-circular orbits instead of plunging.

## E5 — Keplerian initial velocity (the breakthrough)

v0 = vkep * v_circ(r) tangential, where v_circ = sqrt(|F_inward|·r) from the
actual force law. Pure 5-well potential, strength=1, extent ±4.
- friction 0.1: still too chaotic by t≈24.
- friction 0.2, vkep 0.85–1.0: GORGEOUS. Whole-frame coherent differential
  winding (accretion-disk marbling), intact 5-petal flower, ring of eddy
  bullseyes, chaotic filigree bands — matches reference images 2–3.
  Reference img2 ≈ t 6–8, img3 ≈ t 16–24 at these units.
Chosen so far: SPRING=0 STRENGTH=1 FRICTION=0.2 VKEP=1.0, extent ±4 (16:9).
TODO: check swirl handedness vs reference (theirs winds CW inward?),
timing fine-tune, supersampled finals, animation.
