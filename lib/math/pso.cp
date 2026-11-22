// Origin: Pure-CP Stdlib (Zero-Alloc Global Heuristic Optimization: Particle Swarm Optimization PSO & Differential Evolution DE/rand/1/bin) | Created: 2026-11-22

const SPLITMIX_INC: u64 = 0x9E3779B97F4A7C15;
const INV_2_POW_53: f64 = 1.0 / 9007199254740992.0;
const INVALID_SENTINEL: f64 = 1.0e308;

/// Objective function callback: evaluates scalar cost `f(x)` to be minimized.
pub type ObjectiveFn = fn(x: []const f64): f64;

/// Summary returned by `psoMinimize` and `deMinimize`.
pub struct GlobalOptResult {
    best_value: f64,
    iterations: usize,
    evals: usize,
    converged: bool,
}

/// Hyperparameters for Particle Swarm Optimization (linear inertia decay + velocity clamping).
pub struct PsoConfig {
    inertia_start: f64,
    inertia_end: f64,
    cognitive_c1: f64,
    social_c2: f64,
    max_iter: usize,
    tol: f64,
    seed: u64,

    /// Standard balanced PSO defaults (`w: 0.9 -> 0.4`, `c1 = c2 = 1.5`, `max_iter = 500`, `tol = 1e-8`).
    fn default(): PsoConfig {
        return PsoConfig {
            inertia_start: 0.9,
            inertia_end: 0.4,
            cognitive_c1: 1.5,
            social_c2: 1.5,
            max_iter: 500,
            tol: 1.0e-8,
            seed: 42,
        };
    }
}

/// Caller-owned zero-alloc workspace for `psoMinimize`.
/// Required lengths: `pos >= swarm_size * dim`, `vel >= swarm_size * dim`,
/// `pbest_pos >= swarm_size * dim`, `pbest_val >= swarm_size`.
pub struct PsoWorkspace {
    pos: []f64,
    vel: []f64,
    pbest_pos: []f64,
    pbest_val: []f64,
}

/// Hyperparameters for Differential Evolution (`DE/rand/1/bin`).
pub struct DeConfig {
    mutation_f: f64,
    crossover_cr: f64,
    max_iter: usize,
    tol: f64,
    seed: u64,

    /// Classic `DE/rand/1/bin` defaults (`F = 0.8`, `CR = 0.9`, `max_iter = 500`, `tol = 1e-8`).
    fn default(): DeConfig {
        return DeConfig {
            mutation_f: 0.8,
            crossover_cr: 0.9,
            max_iter: 500,
            tol: 1.0e-8,
            seed: 42,
        };
    }
}

/// Caller-owned zero-alloc workspace for `deMinimize`.
/// Required lengths: `pop >= pop_size * dim`, `fitness >= pop_size`, `trial >= dim`.
pub struct DeWorkspace {
    pop: []f64,
    fitness: []f64,
    trial: []f64,
}

struct RngF64 {
    val: f64,
    next_state: u64,
}

struct RngUsize {
    val: usize,
    next_state: u64,
}

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

fn clampF64(v: f64, lo: f64, hi: f64): f64 {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

fn reflectAndClamp(v: f64, lo: f64, hi: f64): f64 {
    let x = v;
    if (x < lo) {
        x = lo + (lo - x);
    } else if (x > hi) {
        x = hi - (x - hi);
    }
    return clampF64(x, lo, hi);
}

fn splitmixMix(x: u64): u64 {
    let z: u64 = x;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn nextUniform01(state: u64): RngF64 {
    const ns: u64 = state +% SPLITMIX_INC;
    const raw: u64 = splitmixMix(ns);
    return RngF64 {
        val: ((raw >> 11) as f64) * INV_2_POW_53,
        next_state: ns,
    };
}

fn nextUsizeBound(state: u64, bound: usize): RngUsize {
    const ns: u64 = state +% SPLITMIX_INC;
    const raw: u64 = splitmixMix(ns);
    const b: u64 = bound as u64;
    return RngUsize {
        val: (raw % b) as usize,
        next_state: ns,
    };
}

fn validateBounds(lower: []const f64, upper: []const f64, best_x_len: usize): bool {
    const dim = lower.len;
    if (dim == 0 || upper.len != dim || best_x_len < dim) {
        return false;
    }
    let j: usize = 0;
    while (j < dim) {
        if (!isFiniteF64(lower[j]) || !isFiniteF64(upper[j]) || lower[j] >= upper[j]) {
            return false;
        }
        j += 1;
    }
    return true;
}

/// Particle Swarm Optimization (PSO) with linear inertia weight decay, velocity clamping,
/// and box-boundary reflection/clamping.
/// Returns `null` on invalid bounds, insufficient workspace, invalid config, or all-non-finite evaluations.
pub fn psoMinimize(
    obj: ObjectiveFn,
    lower: []const f64,
    upper: []const f64,
    swarm_size: usize,
    cfg: PsoConfig,
    best_x_out: *[]f64,
    ws: *PsoWorkspace,
): GlobalOptResult? {
    const dim = lower.len;
    if (swarm_size == 0 || !validateBounds(lower, upper, best_x_out.*.len)) {
        return null;
    }
    if (
        !isFiniteF64(cfg.inertia_start) ||
        !isFiniteF64(cfg.inertia_end) ||
        cfg.inertia_start < 0.0 ||
        cfg.inertia_end < 0.0 ||
        !isFiniteF64(cfg.cognitive_c1) ||
        cfg.cognitive_c1 < 0.0 ||
        !isFiniteF64(cfg.social_c2) ||
        cfg.social_c2 < 0.0 ||
        !isFiniteF64(cfg.tol) ||
        cfg.tol < 0.0
    ) {
        return null;
    }
    if (
        ws.pos.len / dim < swarm_size ||
        ws.vel.len / dim < swarm_size ||
        ws.pbest_pos.len / dim < swarm_size ||
        ws.pbest_val.len < swarm_size
    ) {
        return null;
    }

    let rng_state: u64 = cfg.seed;
    let evals: usize = 0;
    let has_gbest: bool = false;
    let gbest_val: f64 = INVALID_SENTINEL;

    // 1. Initialize swarm positions, velocities, personal bests, and global best.
    let i: usize = 0;
    while (i < swarm_size) {
        const base = i * dim;
        let j: usize = 0;
        while (j < dim) {
            const span = upper[j] - lower[j];
            const u_pos = nextUniform01(rng_state);
            rng_state = u_pos.next_state;
            const u_vel = nextUniform01(rng_state);
            rng_state = u_vel.next_state;

            const x_init = clampF64(lower[j] + u_pos.val * span, lower[j], upper[j]);
            const v_init = (2.0 * u_vel.val - 1.0) * 0.2 * span;
            ws.pos[base + j] = x_init;
            ws.vel[base + j] = v_init;
            ws.pbest_pos[base + j] = x_init;
            j += 1;
        }

        const fv = obj(ws.pos[base..base + dim]);
        evals += 1;
        if (isFiniteF64(fv)) {
            ws.pbest_val[i] = fv;
            if (!has_gbest || fv < gbest_val) {
                has_gbest = true;
                gbest_val = fv;
                j = 0;
                while (j < dim) {
                    best_x_out.*[j] = ws.pos[base + j];
                    j += 1;
                }
            }
        } else {
            ws.pbest_val[i] = INVALID_SENTINEL;
        }
        i += 1;
    }

    if (!has_gbest) {
        return null;
    }

    let iterations: usize = 0;
    let converged: bool = false;

    // 2. Main PSO iteration loop.
    let iter: usize = 0;
    while (iter < cfg.max_iter) {
        const w = if (cfg.max_iter > 1)
            cfg.inertia_start - (cfg.inertia_start - cfg.inertia_end) * ((iter as f64) / ((cfg.max_iter - 1) as f64))
        else
            cfg.inertia_end;

        i = 0;
        while (i < swarm_size) {
            const base = i * dim;
            let j: usize = 0;
            while (j < dim) {
                const idx = base + j;
                const lo = lower[j];
                const hi = upper[j];
                const v_max = 0.5 * (hi - lo);

                const r1_s = nextUniform01(rng_state);
                rng_state = r1_s.next_state;
                const r2_s = nextUniform01(rng_state);
                rng_state = r2_s.next_state;

                const cur_x = ws.pos[idx];
                let v_new = w * ws.vel[idx] +
                    cfg.cognitive_c1 * r1_s.val * (ws.pbest_pos[idx] - cur_x) +
                    cfg.social_c2 * r2_s.val * (best_x_out.*[j] - cur_x);
                v_new = clampF64(v_new, -v_max, v_max);

                let x_new = cur_x + v_new;
                if (x_new < lo) {
                    x_new = clampF64(lo + (lo - x_new), lo, hi);
                    v_new = -0.5 * v_new;
                } else if (x_new > hi) {
                    x_new = clampF64(hi - (x_new - hi), lo, hi);
                    v_new = -0.5 * v_new;
                }

                ws.vel[idx] = v_new;
                ws.pos[idx] = x_new;
                j += 1;
            }

            const fv = obj(ws.pos[base..base + dim]);
            evals += 1;
            if (isFiniteF64(fv)) {
                if (fv < ws.pbest_val[i]) {
                    ws.pbest_val[i] = fv;
                    j = 0;
                    while (j < dim) {
                        ws.pbest_pos[base + j] = ws.pos[base + j];
                        j += 1;
                    }
                }
                if (fv < gbest_val) {
                    gbest_val = fv;
                    j = 0;
                    while (j < dim) {
                        best_x_out.*[j] = ws.pos[base + j];
                        j += 1;
                    }
                }
            }
            i += 1;
        }

        iterations = iter + 1;

        // Convergence check: both pbest spread and swarm spatial radius within tolerance.
        if (cfg.tol > 0.0 && swarm_size > 1) {
            let max_pbest: f64 = gbest_val;
            let max_radius: f64 = 0.0;
            let all_valid: bool = true;
            i = 0;
            while (i < swarm_size) {
                if (ws.pbest_val[i] >= INVALID_SENTINEL * 0.5) {
                    all_valid = false;
                    break;
                }
                if (ws.pbest_val[i] > max_pbest) {
                    max_pbest = ws.pbest_val[i];
                }
                const base = i * dim;
                let j: usize = 0;
                while (j < dim) {
                    const d = absF64(ws.pos[base + j] - best_x_out.*[j]);
                    if (d > max_radius) {
                        max_radius = d;
                    }
                    j += 1;
                }
                i += 1;
            }
            if (all_valid && (max_pbest - gbest_val) <= cfg.tol && max_radius <= cfg.tol) {
                converged = true;
                break;
            }
        }

        iter += 1;
    }

    return GlobalOptResult {
        best_value: gbest_val,
        iterations: iterations,
        evals: evals,
        converged: converged,
    };
}

/// Differential Evolution (`DE/rand/1/bin`) global minimizer with box-boundary reflection/clamping.
/// Requires `pop_size >= 4`. Returns `null` on invalid inputs, insufficient workspace, or all-non-finite evaluations.
pub fn deMinimize(
    obj: ObjectiveFn,
    lower: []const f64,
    upper: []const f64,
    pop_size: usize,
    cfg: DeConfig,
    best_x_out: *[]f64,
    ws: *DeWorkspace,
): GlobalOptResult? {
    const dim = lower.len;
    if (pop_size < 4 || !validateBounds(lower, upper, best_x_out.*.len)) {
        return null;
    }
    if (
        !isFiniteF64(cfg.mutation_f) ||
        cfg.mutation_f <= 0.0 ||
        cfg.mutation_f > 2.0 ||
        !isFiniteF64(cfg.crossover_cr) ||
        cfg.crossover_cr < 0.0 ||
        cfg.crossover_cr > 1.0 ||
        !isFiniteF64(cfg.tol) ||
        cfg.tol < 0.0
    ) {
        return null;
    }
    if (
        ws.pop.len / dim < pop_size ||
        ws.fitness.len < pop_size ||
        ws.trial.len < dim
    ) {
        return null;
    }

    let rng_state: u64 = cfg.seed;
    let evals: usize = 0;
    let has_best: bool = false;
    let best_val: f64 = INVALID_SENTINEL;

    // 1. Initialize population uniformly inside [lower, upper].
    let i: usize = 0;
    while (i < pop_size) {
        const base = i * dim;
        let j: usize = 0;
        while (j < dim) {
            const u = nextUniform01(rng_state);
            rng_state = u.next_state;
            ws.pop[base + j] = clampF64(lower[j] + u.val * (upper[j] - lower[j]), lower[j], upper[j]);
            j += 1;
        }
        const fv = obj(ws.pop[base..base + dim]);
        evals += 1;
        if (isFiniteF64(fv)) {
            ws.fitness[i] = fv;
            if (!has_best || fv < best_val) {
                has_best = true;
                best_val = fv;
                j = 0;
                while (j < dim) {
                    best_x_out.*[j] = ws.pop[base + j];
                    j += 1;
                }
            }
        } else {
            ws.fitness[i] = INVALID_SENTINEL;
        }
        i += 1;
    }

    if (!has_best) {
        return null;
    }

    let iterations: usize = 0;
    let converged: bool = false;

    // 2. Main DE/rand/1/bin generation loop.
    let iter: usize = 0;
    while (iter < cfg.max_iter) {
        i = 0;
        while (i < pop_size) {
            // Pick distinct r1, r2, r3 != i.
            let r1: usize = i;
            while (r1 == i) {
                const s = nextUsizeBound(rng_state, pop_size);
                rng_state = s.next_state;
                r1 = s.val;
            }
            let r2: usize = i;
            while (r2 == i || r2 == r1) {
                const s = nextUsizeBound(rng_state, pop_size);
                rng_state = s.next_state;
                r2 = s.val;
            }
            let r3: usize = i;
            while (r3 == i || r3 == r1 || r3 == r2) {
                const s = nextUsizeBound(rng_state, pop_size);
                rng_state = s.next_state;
                r3 = s.val;
            }

            const jr_s = nextUsizeBound(rng_state, dim);
            rng_state = jr_s.next_state;
            const j_rand = jr_s.val;

            const base_i = i * dim;
            const base_1 = r1 * dim;
            const base_2 = r2 * dim;
            const base_3 = r3 * dim;

            // Binomial crossover + rand/1 mutation.
            let j: usize = 0;
            while (j < dim) {
                const u_cr = nextUniform01(rng_state);
                rng_state = u_cr.next_state;
                if (j == j_rand || u_cr.val < cfg.crossover_cr) {
                    const mutant = ws.pop[base_1 + j] + cfg.mutation_f * (ws.pop[base_2 + j] - ws.pop[base_3 + j]);
                    ws.trial[j] = reflectAndClamp(mutant, lower[j], upper[j]);
                } else {
                    ws.trial[j] = ws.pop[base_i + j];
                }
                j += 1;
            }

            const trial_val = obj(ws.trial[0..dim]);
            evals += 1;
            if (isFiniteF64(trial_val) && trial_val <= ws.fitness[i]) {
                ws.fitness[i] = trial_val;
                j = 0;
                while (j < dim) {
                    ws.pop[base_i + j] = ws.trial[j];
                    j += 1;
                }
                if (trial_val < best_val) {
                    best_val = trial_val;
                    j = 0;
                    while (j < dim) {
                        best_x_out.*[j] = ws.trial[j];
                        j += 1;
                    }
                }
            }
            i += 1;
        }

        iterations = iter + 1;

        // Convergence check: both fitness spread and spatial radius within tolerance.
        if (cfg.tol > 0.0) {
            let max_fit: f64 = best_val;
            let max_radius: f64 = 0.0;
            let all_valid: bool = true;
            i = 0;
            while (i < pop_size) {
                if (ws.fitness[i] >= INVALID_SENTINEL * 0.5) {
                    all_valid = false;
                    break;
                }
                if (ws.fitness[i] > max_fit) {
                    max_fit = ws.fitness[i];
                }
                const base = i * dim;
                let j: usize = 0;
                while (j < dim) {
                    const d = absF64(ws.pop[base + j] - best_x_out.*[j]);
                    if (d > max_radius) {
                        max_radius = d;
                    }
                    j += 1;
                }
                i += 1;
            }
            if (all_valid && (max_fit - best_val) <= cfg.tol && max_radius <= cfg.tol) {
                converged = true;
                break;
            }
        }

        iter += 1;
    }

    return GlobalOptResult {
        best_value: best_val,
        iterations: iterations,
        evals: evals,
        converged: converged,
    };
}
