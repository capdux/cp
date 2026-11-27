// Origin: Pure-CP Stdlib (Zero-Alloc Simulated Annealing: Continuous Box-Constrained & Permutation 2-opt/Swap) | Created: 2026-11-22

const PI: f64 = 3.14159265358979323846;
const SPLITMIX_INC: u64 = 0x9E3779B97F4A7C15;
const INV_2_POW_53: f64 = 1.0 / 9007199254740992.0;

/// Temperature annealing schedule.
pub enum CoolingSchedule {
    Geometric,
    Linear,
    Fast,
    Logarithmic,
}

/// Objective energy callback for continuous optimization.
pub type RealEnergyFn = fn(x: []const f64): f64;

/// Objective energy callback for permutation optimization (e.g. TSP, QAP).
pub type PermEnergyFn = fn(perm: []const usize): f64;

/// Hyperparameters for `annealContinuous` and `annealPermutation`.
pub struct AnnealConfig {
    t_init: f64,
    t_min: f64,
    cooling_rate: f64,
    step_scale: f64,
    max_steps: usize,
    schedule: CoolingSchedule,
    seed: u64,

    /// Balanced defaults (`t_init=10.0`, `t_min=1e-5`, `cooling_rate=0.995`, `step_scale=0.25`, `max_steps=5000`).
    static fn default(): AnnealConfig {
        return AnnealConfig {
            t_init: 10.0,
            t_min: 1.0e-5,
            cooling_rate: 0.995,
            step_scale: 0.25,
            max_steps: 5000,
            schedule: CoolingSchedule.Geometric,
            seed: 42,
        };
    }
}

/// Summary returned by `annealContinuous` and `annealPermutation`.
pub struct AnnealResult {
    best_energy: f64,
    final_energy: f64,
    accepted: usize,
    steps: usize,
    final_temp: f64,
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

fn nextNormal01(state: u64): RngF64 {
    const s1 = nextUniform01(state);
    const s2 = nextUniform01(s1.next_state);
    const u_a = if (s1.val > 1.0e-15) s1.val else 1.0e-15;
    const mag = @sqrt(-2.0 * @log(u_a));
    return RngF64 {
        val: mag * @cos(2.0 * PI * s2.val),
        next_state: s2.next_state,
    };
}

fn nextUsizeBound(state: u64, bound: usize): RngUsize {
    const ns: u64 = state +% SPLITMIX_INC;
    const raw: u64 = splitmixMix(ns);
    return RngUsize {
        val: (raw % (bound as u64)) as usize,
        next_state: ns,
    };
}

/// Evaluates annealing temperature at iteration `step` under the chosen `CoolingSchedule`.
pub fn temperatureAt(
    t0: f64,
    t_min: f64,
    alpha: f64,
    step: usize,
    max_steps: usize,
    schedule: CoolingSchedule,
): f64 {
    if (!isFiniteF64(t0) || t0 <= 0.0) {
        return 0.0;
    }
    let t_lo: f64 = 0.0;
    if (isFiniteF64(t_min) && t_min > 0.0) {
        t_lo = if (t_min > t0) t0 else t_min;
    }

    const k = step as f64;
    let temp: f64 = t0;
    match (schedule) {
        CoolingSchedule.Geometric => {
            if (!isFiniteF64(alpha) || alpha <= 0.0) {
                temp = t_lo;
            } else if (alpha >= 1.0) {
                temp = t0;
            } else {
                temp = t0 * @exp(k * @log(alpha));
            }
        }
        CoolingSchedule.Linear => {
            if (max_steps == 0) {
                temp = t_lo;
            } else {
                const raw_frac = k / (max_steps as f64);
                const frac = if (raw_frac > 1.0) 1.0 else raw_frac;
                temp = t0 - (t0 - t_lo) * frac;
            }
        }
        CoolingSchedule.Fast => {
            temp = t0 / (1.0 + k);
        }
        CoolingSchedule.Logarithmic => {
            temp = t0 / (1.0 + @log(1.0 + k));
        }
    }

    if (!isFiniteF64(temp) || temp < t_lo) {
        return t_lo;
    }
    if (temp > t0) {
        return t0;
    }
    return temp;
}

/// Metropolis-Hastings acceptance criterion:
/// always accepts `delta_energy <= 0`, otherwise accepts with probability `exp(-delta_energy / temp)`.
pub fn metropolisAccept(delta_energy: f64, temp: f64, u_rand: f64): bool {
    if (!isFiniteF64(delta_energy) || !isFiniteF64(temp) || !isFiniteF64(u_rand)) {
        return false;
    }
    if (delta_energy <= 0.0) {
        return true;
    }
    if (temp <= 0.0) {
        return false;
    }
    const prob = @exp(-delta_energy / temp);
    return u_rand < prob;
}

fn validateConfig(cfg: AnnealConfig, require_pos_step: bool): bool {
    if (
        !isFiniteF64(cfg.t_init) ||
        cfg.t_init <= 0.0 ||
        !isFiniteF64(cfg.t_min) ||
        cfg.t_min < 0.0 ||
        cfg.t_min > cfg.t_init ||
        !isFiniteF64(cfg.cooling_rate) ||
        cfg.cooling_rate <= 0.0 ||
        cfg.cooling_rate > 1.0 ||
        !isFiniteF64(cfg.step_scale)
    ) {
        return false;
    }
    if (require_pos_step && cfg.step_scale <= 0.0) {
        return false;
    }
    if (!require_pos_step && cfg.step_scale < 0.0) {
        return false;
    }
    return true;
}

/// Box-constrained continuous Simulated Annealing with temperature-adaptive Gaussian proposals.
/// Returns `null` on invalid bounds/config, insufficient buffers, or non-finite initial energy.
pub fn annealContinuous(
    energy_fn: RealEnergyFn,
    x0: []const f64,
    lower: []const f64,
    upper: []const f64,
    cfg: AnnealConfig,
    best_x_out: *[]f64,
    curr_s: *[]f64,
    cand_s: *[]f64,
): AnnealResult? {
    const dim = x0.len;
    if (
        dim == 0 ||
        lower.len != dim ||
        upper.len != dim ||
        best_x_out.*.len < dim ||
        curr_s.*.len < dim ||
        cand_s.*.len < dim ||
        !validateConfig(cfg, true)
    ) {
        return null;
    }

    let j: usize = 0;
    while (j < dim) {
        if (!isFiniteF64(x0[j]) || !isFiniteF64(lower[j]) || !isFiniteF64(upper[j]) || lower[j] >= upper[j]) {
            return null;
        }
        const clamped = clampF64(x0[j], lower[j], upper[j]);
        curr_s.*[j] = clamped;
        best_x_out.*[j] = clamped;
        j += 1;
    }

    let e_curr = energy_fn(curr_s.*[0..dim]);
    if (!isFiniteF64(e_curr)) {
        return null;
    }
    let e_best = e_curr;
    let accepted: usize = 0;
    let rng_state: u64 = cfg.seed;
    let final_temp: f64 = cfg.t_init;

    let step: usize = 0;
    while (step < cfg.max_steps) {
        const temp = temperatureAt(cfg.t_init, cfg.t_min, cfg.cooling_rate, step, cfg.max_steps, cfg.schedule);
        final_temp = temp;

        let scale_ratio = @sqrt(temp / cfg.t_init);
        if (scale_ratio < 1.0e-5) {
            scale_ratio = 1.0e-5;
        }

        if (dim == 1 || (step & 1) == 0) {
            // Full-dimensional Gaussian proposal
            j = 0;
            while (j < dim) {
                const g = nextNormal01(rng_state);
                rng_state = g.next_state;
                const step_w = cfg.step_scale * scale_ratio * (upper[j] - lower[j]);
                cand_s.*[j] = reflectAndClamp(curr_s.*[j] + g.val * step_w, lower[j], upper[j]);
                j += 1;
            }
        } else {
            // Single-coordinate Gaussian proposal for fine high-dimensional exploitation
            j = 0;
            while (j < dim) {
                cand_s.*[j] = curr_s.*[j];
                j += 1;
            }
            const dim_s = nextUsizeBound(rng_state, dim);
            rng_state = dim_s.next_state;
            const d_idx = dim_s.val;
            const g = nextNormal01(rng_state);
            rng_state = g.next_state;
            const step_w = cfg.step_scale * scale_ratio * (upper[d_idx] - lower[d_idx]);
            cand_s.*[d_idx] = reflectAndClamp(curr_s.*[d_idx] + g.val * step_w, lower[d_idx], upper[d_idx]);
        }

        const e_cand = energy_fn(cand_s.*[0..dim]);
        if (isFiniteF64(e_cand)) {
            const u_s = nextUniform01(rng_state);
            rng_state = u_s.next_state;
            if (metropolisAccept(e_cand - e_curr, temp, u_s.val)) {
                j = 0;
                while (j < dim) {
                    curr_s.*[j] = cand_s.*[j];
                    j += 1;
                }
                e_curr = e_cand;
                accepted += 1;
                if (e_cand < e_best) {
                    j = 0;
                    while (j < dim) {
                        best_x_out.*[j] = cand_s.*[j];
                        j += 1;
                    }
                    e_best = e_cand;
                }
            }
        }
        step += 1;
    }

    return AnnealResult {
        best_energy: e_best,
        final_energy: e_curr,
        accepted: accepted,
        steps: cfg.max_steps,
        final_temp: final_temp,
    };
}

/// Discrete permutation Simulated Annealing using hybrid 2-opt segment reversal (80%) and random swap (20%).
/// Validates that `init_perm` is a valid permutation of `0..n-1`.
pub fn annealPermutation(
    energy_fn: PermEnergyFn,
    init_perm: []const usize,
    cfg: AnnealConfig,
    best_perm_out: *[]usize,
    curr_s: *[]usize,
    cand_s: *[]usize,
): AnnealResult? {
    const n = init_perm.len;
    if (
        n == 0 ||
        best_perm_out.*.len < n ||
        curr_s.*.len < n ||
        cand_s.*.len < n ||
        !validateConfig(cfg, false)
    ) {
        return null;
    }

    // Zero-alloc permutation validation using cand_s as temporary seen marker array.
    let i: usize = 0;
    while (i < n) {
        cand_s.*[i] = 0 as usize;
        i += 1;
    }
    i = 0;
    while (i < n) {
        const val = init_perm[i];
        if (val >= n || cand_s.*[val] != 0) {
            return null;
        }
        cand_s.*[val] = 1 as usize;
        curr_s.*[i] = val;
        best_perm_out.*[i] = val;
        i += 1;
    }

    let e_curr = energy_fn(curr_s.*[0..n]);
    if (!isFiniteF64(e_curr)) {
        return null;
    }
    let e_best = e_curr;
    if (n < 2) {
        return AnnealResult {
            best_energy: e_best,
            final_energy: e_curr,
            accepted: 0,
            steps: 0,
            final_temp: cfg.t_init,
        };
    }

    let accepted: usize = 0;
    let rng_state: u64 = cfg.seed;
    let final_temp: f64 = cfg.t_init;

    let step: usize = 0;
    while (step < cfg.max_steps) {
        const temp = temperatureAt(cfg.t_init, cfg.t_min, cfg.cooling_rate, step, cfg.max_steps, cfg.schedule);
        final_temp = temp;

        let k: usize = 0;
        while (k < n) {
            cand_s.*[k] = curr_s.*[k];
            k += 1;
        }

        // Sample two distinct indices a < b.
        const s1 = nextUsizeBound(rng_state, n);
        rng_state = s1.next_state;
        const s2 = nextUsizeBound(rng_state, n - 1);
        rng_state = s2.next_state;
        let a = s1.val;
        let b = if (s2.val >= a) s2.val + 1 else s2.val;
        if (a > b) {
            const tmp = a;
            a = b;
            b = tmp;
        }

        const mode_s = nextUniform01(rng_state);
        rng_state = mode_s.next_state;
        if (mode_s.val < 0.8) {
            // 2-opt segment reversal on [a..=b]
            let lo = a;
            let hi = b;
            while (lo < hi) {
                const tmp = cand_s.*[lo];
                cand_s.*[lo] = cand_s.*[hi];
                cand_s.*[hi] = tmp;
                lo += 1;
                hi -= 1;
            }
        } else {
            // Random pair swap
            const tmp = cand_s.*[a];
            cand_s.*[a] = cand_s.*[b];
            cand_s.*[b] = tmp;
        }

        const e_cand = energy_fn(cand_s.*[0..n]);
        if (isFiniteF64(e_cand)) {
            const u_s = nextUniform01(rng_state);
            rng_state = u_s.next_state;
            if (metropolisAccept(e_cand - e_curr, temp, u_s.val)) {
                k = 0;
                while (k < n) {
                    curr_s.*[k] = cand_s.*[k];
                    k += 1;
                }
                e_curr = e_cand;
                accepted += 1;
                if (e_cand < e_best) {
                    k = 0;
                    while (k < n) {
                        best_perm_out.*[k] = cand_s.*[k];
                        k += 1;
                    }
                    e_best = e_cand;
                }
            }
        }
        step += 1;
    }

    return AnnealResult {
        best_energy: e_best,
        final_energy: e_curr,
        accepted: accepted,
        steps: cfg.max_steps,
        final_temp: final_temp,
    };
}
