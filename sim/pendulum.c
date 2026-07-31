/*
 * Magnetic pendulum simulator — fractal basins of attraction.
 *
 * Every pixel of a W x H grid is an independent particle. Its initial
 * position is the pixel's world coordinate (initial velocity is zero, or
 * optionally a rigid rotation field). The particle moves in the field of
 * N point attractors ("magnets") placed on a regular N-gon, plus a
 * restoring spring toward the origin and linear friction:
 *
 *   r'' = -friction * r' - spring * r
 *         + strength * sum_i (m_i - r) / (|m_i - r|^2 + d^2)^(3/2)
 *
 * Integration is classic RK4 with a fixed timestep. At each requested
 * snapshot time the current (x, y) of every particle is written as raw
 * little-endian float32 pairs to <prefix>_t<time>.xy
 *
 * Usage:
 *   pendulum W H XMIN XMAX YMIN YMAX DT PREFIX t0 [t1 t2 ...]
 * Environment overrides:
 *   FRICTION SPRING STRENGTH HEIGHT NMAG MAGRADIUS PHASE VROT
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <omp.h>

typedef double real;

static real env_real(const char *name, real def) {
    const char *s = getenv(name);
    return s ? atof(s) : def;
}
static int env_int(const char *name, int def) {
    const char *s = getenv(name);
    return s ? atoi(s) : def;
}

static int    NMAG;
static real   MX[16], MY[16];
static real   FRICTION, SPRING, STRENGTH, D2;
static real   CENTRAL, CD2;   /* broad central well: -C r/(r^2+D^2)^{3/2} */
static real   BGA, BGD2;      /* log-potential bg: -A r/(r^2+D^2) — flat v_circ */

static inline void accel(real x, real y, real vx, real vy,
                         real *ax, real *ay) {
    real fx = -FRICTION * vx - SPRING * x;
    real fy = -FRICTION * vy - SPRING * y;
    if (BGA != 0.0) {
        real inv = BGA / (x * x + y * y + BGD2);
        fx -= x * inv;
        fy -= y * inv;
    }
    if (CENTRAL != 0.0) {
        real rr = x * x + y * y + CD2;
        real inv = CENTRAL / (rr * sqrt(rr));
        fx -= x * inv;
        fy -= y * inv;
    }
    for (int i = 0; i < NMAG; i++) {
        real dx = MX[i] - x, dy = MY[i] - y;
        real dd = dx * dx + dy * dy + D2;
        real inv = STRENGTH / (dd * sqrt(dd));
        fx += dx * inv;
        fy += dy * inv;
    }
    *ax = fx;
    *ay = fy;
}

int main(int argc, char **argv) {
    if (argc < 10) {
        fprintf(stderr, "usage: %s W H XMIN XMAX YMIN YMAX DT PREFIX t0 [t1 ...]\n", argv[0]);
        return 1;
    }
    int W = atoi(argv[1]), H = atoi(argv[2]);
    real xmin = atof(argv[3]), xmax = atof(argv[4]);
    real ymin = atof(argv[5]), ymax = atof(argv[6]);
    real dt = atof(argv[7]);
    const char *prefix = argv[8];
    int nsnap = argc - 9;
    real *tsnap = malloc(sizeof(real) * nsnap);
    for (int i = 0; i < nsnap; i++) tsnap[i] = atof(argv[9 + i]);

    FRICTION  = env_real("FRICTION", 0.2);
    SPRING    = env_real("SPRING", 0.5);
    STRENGTH  = env_real("STRENGTH", 1.0);
    real d    = env_real("HEIGHT", 0.25);
    D2        = d * d;
    NMAG      = env_int("NMAG", 5);
    real mrad = env_real("MAGRADIUS", 1.0);
    real phase= env_real("PHASE", M_PI / 2);   /* first magnet points up */
    real vrot = env_real("VROT", 0.0);         /* v0 = vrot * (-y, x)    */
    CENTRAL   = env_real("CENTRAL", 0.0);
    real cd   = env_real("CD", 2.0);
    CD2       = cd * cd;
    real vkep = env_real("VKEP", 0.0);  /* v0 = vkep * v_circ(r) tangential */
    BGA       = env_real("BGA", 0.0);
    real bgd  = env_real("BGD", 1.0);
    BGD2      = bgd * bgd;

    for (int i = 0; i < NMAG; i++) {
        real a = phase + 2.0 * M_PI * i / NMAG;
        MX[i] = mrad * cos(a);
        MY[i] = mrad * sin(a);
    }

    long n = (long)W * H;
    real *x  = malloc(n * sizeof(real));
    real *y  = malloc(n * sizeof(real));
    real *vx = malloc(n * sizeof(real));
    real *vy = malloc(n * sizeof(real));

    #pragma omp parallel for
    for (long p = 0; p < n; p++) {
        long i = p % W, j = p / W;
        /* pixel centers; row 0 = top of image = ymax */
        real px = xmin + (xmax - xmin) * ((i + 0.5) / W);
        real py = ymax - (ymax - ymin) * ((j + 0.5) / H);
        x[p] = px;  y[p] = py;
        if (vkep != 0.0) {
            /* local circular-orbit speed: v = sqrt(|F_inward| * r) */
            real ax, ay;
            accel(px, py, 0, 0, &ax, &ay);   /* v=0: friction term vanishes */
            real r = sqrt(px * px + py * py) + 1e-12;
            real finw = -(ax * px + ay * py) / r;   /* inward component */
            real v = vkep * sqrt(fmax(finw, 0.0) * r);
            vx[p] = -v * py / r;
            vy[p] =  v * px / r;
        } else {
            vx[p] = -vrot * py;
            vy[p] =  vrot * px;
        }
    }

    fprintf(stderr, "grid %dx%d  n=%ld  dt=%g  friction=%g spring=%g strength=%g d=%g nmag=%d mrad=%g\n",
            W, H, n, dt, FRICTION, SPRING, STRENGTH, d, NMAG, mrad);

    real t = 0.0;
    float *buf = malloc(n * 2 * sizeof(float));

    for (int s = 0; s < nsnap; s++) {
        long steps = lround((tsnap[s] - t) / dt);
        double t0 = omp_get_wtime();
        #pragma omp parallel for schedule(static)
        for (long p = 0; p < n; p++) {
            real X = x[p], Y = y[p], VX = vx[p], VY = vy[p];
            for (long k = 0; k < steps; k++) {
                real ax1, ay1, ax2, ay2, ax3, ay3, ax4, ay4;
                accel(X, Y, VX, VY, &ax1, &ay1);
                real x2 = X + 0.5 * dt * VX, y2 = Y + 0.5 * dt * VY;
                real vx2 = VX + 0.5 * dt * ax1, vy2 = VY + 0.5 * dt * ay1;
                accel(x2, y2, vx2, vy2, &ax2, &ay2);
                real x3 = X + 0.5 * dt * vx2, y3 = Y + 0.5 * dt * vy2;
                real vx3 = VX + 0.5 * dt * ax2, vy3 = VY + 0.5 * dt * ay2;
                accel(x3, y3, vx3, vy3, &ax3, &ay3);
                real x4 = X + dt * vx3, y4 = Y + dt * vy3;
                real vx4 = VX + dt * ax3, vy4 = VY + dt * ay3;
                accel(x4, y4, vx4, vy4, &ax4, &ay4);
                X  += dt / 6.0 * (VX + 2 * vx2 + 2 * vx3 + vx4);
                Y  += dt / 6.0 * (VY + 2 * vy2 + 2 * vy3 + vy4);
                VX += dt / 6.0 * (ax1 + 2 * ax2 + 2 * ax3 + ax4);
                VY += dt / 6.0 * (ay1 + 2 * ay2 + 2 * ay3 + ay4);
            }
            x[p] = X; y[p] = Y; vx[p] = VX; vy[p] = VY;
            buf[2 * p]     = (float)X;
            buf[2 * p + 1] = (float)Y;
        }
        t += steps * dt;
        char fname[512];
        snprintf(fname, sizeof fname, "%s_t%08.3f.xy", prefix, t);
        FILE *f = fopen(fname, "wb");
        fwrite(buf, sizeof(float), n * 2, f);
        fclose(f);
        fprintf(stderr, "t=%8.3f  (%ld steps, %.1fs)  -> %s\n",
                t, steps, omp_get_wtime() - t0, fname);
    }
    return 0;
}
