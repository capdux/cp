// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Multi-Armed Bandits: UCB1, Epsilon-Greedy, Thompson Sampling & EXP3)

import std from "std";

const PI: f64 = 3.14159265358979323846;
const LN2: f64 = 0.69314718055994530942;
const SPLITMIX_INC: u64 = 0x9E3779B97F4A7C15;
const INV_2_POW_53: f64 = 1.0 / 9007199254740992.0;

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn splitmixMix(x: u64): u64 {
    let z: u64 = x;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn u64ToFloat01(r: u64): f64 {
    return ((r >> 11) as f64) * INV_2_POW_53;
}

/// Pure-CP double-precision natural logarithm for `x > 0` (~1e-16 accuracy).
fn lnF64(x: f64): f64 {
    let m: f64 = x;
    let k: i32 = 0;
    while (m >= 1.5) {
        m *= 0.5;
        k += 1;
    }
    while (m < 0.75) {
        m *= 2.0;
        k -= 1;
    }
    const t: f64 = (m - 1.0) / (m + 1.0);
    const t2: f64 = t * t;
    let term: f64 = t;
    let sum: f64 = 0.0;
    let denom: f64 = 1.0;
    let iter: usize = 0;
    while (iter < 12) {
        sum += term / denom;
        term *= t2;
        denom += 2.0;
        iter += 1;
    }
    return 2.0 * sum + (k as f64) * LN2;
}

/// Pure value-returning RNG helper to avoid mutable scalar pointer borrow issues.
struct RngSample {
    val: f64,
    next_state: u64,
}

fn nextUniform01(state: u64): RngSample {
    const ns: u64 = state +% SPLITMIX_INC;
    return RngSample {
        val: u64ToFloat01(splitmixMix(ns)),
        next_state: ns,
    };
}

fn nextNormal01(state: u64): RngSample {
    const s1: RngSample = nextUniform01(state);
    const s2: RngSample = nextUniform01(s1.next_state);
    const u_a: f64 = if (s1.val > 1.0e-16) s1.val else 1.0e-16;
    const mag: f64 = std.math.sqrt(-2.0 * lnF64(u_a));
    const z: f64 = mag * std.math.cos(2.0 * PI * s2.val);
    return RngSample {
        val: z,
        next_state: s2.next_state,
    };
}

/// Marsaglia-Tsang Gamma(shape, 1) sampler for `shape > 0`.
fn sampleGamma(shape: f64, state: u64): RngSample {
    if (shape < 1.0) {
        const g: RngSample = sampleGamma(shape + 1.0, state);
        const u_s: RngSample = nextUniform01(g.next_state);
        const u: f64 = if (u_s.val > 1.0e-16) u_s.val else 1.0e-16;
        return RngSample {
            val: g.val * std.math.exp(lnF64(u) / shape),
            next_state: u_s.next_state,
        };
    }

    const d: f64 = shape - (1.0 / 3.0);
    const c: f64 = 1.0 / std.math.sqrt(9.0 * d);
    let st: u64 = state;
    let guard: usize = 0;
    while (guard < 1000) {
        const zs: RngSample = nextNormal01(st);
        st = zs.next_state;
        const z: f64 = zs.val;
        const base: f64 = 1.0 + c * z;
        if (base > 0.0) {
            const v: f64 = base * base * base;
            const us: RngSample = nextUniform01(st);
            st = us.next_state;
            const u: f64 = if (us.val > 1.0e-16) us.val else 1.0e-16;
            const z2: f64 = z * z;
            if (u < (1.0 - 0.0331 * z2 * z2)) {
                return RngSample { val: d * v, next_state: st };
            }
            if (lnF64(u) < (0.5 * z2 + d * (1.0 - v + lnF64(v)))) {
                return RngSample { val: d * v, next_state: st };
            }
        }
        guard += 1;
    }
    return RngSample { val: shape, next_state: st };
}

fn sampleBeta(alpha: f64, beta_p: f64, state: u64): RngSample {
    const gx: RngSample = sampleGamma(alpha, state);
    const gy: RngSample = sampleGamma(beta_p, gx.next_state);
    const denom: f64 = gx.val + gy.val;
    const res: f64 = if (denom > 0.0) (gx.val / denom) else (alpha / (alpha + beta_p));
    return RngSample { val: res, next_state: gy.next_state };
}

/// Deterministic UCB1 (Upper Confidence Bound) multi-armed bandit.
/// Pulls every arm once during cold-start before optimizing `mean + c * sqrt(ln(N) / n_i)`.
pub struct Ucb1Bandit {
    num_arms: usize,
    counts: []u64,
    means: []f64,
    total_pulls: u64,
    c: f64,

    /// Initializes a UCB1 bandit over caller-owned buffers `counts` and `means`.
    /// Requires `counts.len > 0`, `means.len >= counts.len`, and finite `c >= 0`.
    static fn init(counts: []u64, means: []f64, c: f64): Ucb1Bandit? {
        const k: usize = counts.len;
        if (k == 0 || means.len < k || !isFiniteF64(c) || c < 0.0) {
            return null;
        }
        let i: usize = 0;
        while (i < k) {
            counts[i] = 0;
            means[i] = 0.0;
            i += 1;
        }
        return Ucb1Bandit {
            num_arms: k,
            counts: counts,
            means: means,
            total_pulls: 0,
            c: c,
        };
    }

    /// Returns the UCB1 optimism score for `arm` (`+inf` if unpulled, `-inf` if out of bounds).
    fn armScore(arm: usize): f64 {
        if (arm >= _.num_arms) {
            return 0.0 - std.math.inf(f64);
        }
        const cnt: u64 = _.counts[arm];
        if (cnt == 0) {
            return std.math.inf(f64);
        }
        const total_f: f64 = if (_.total_pulls > 1) (_.total_pulls as f64) else 1.0;
        const bonus: f64 = _.c * std.math.sqrt(lnF64(total_f) / (cnt as f64));
        return _.means[arm] + bonus;
    }

    /// Selects the next arm to pull. Guarantees every arm is pulled once before UCB comparison.
    fn selectArm(): usize? {
        if (_.num_arms == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < _.num_arms) {
            if (_.counts[i] == 0) {
                return i;
            }
            i += 1;
        }
        let best_arm: usize = 0;
        let best_score: f64 = _.armScore(0);
        let j: usize = 1;
        while (j < _.num_arms) {
            const sc: f64 = _.armScore(j);
            if (sc > best_score) {
                best_score = sc;
                best_arm = j;
            }
            j += 1;
        }
        return best_arm;
    }

    /// Updates empirical mean and pull count for `arm` with observed `reward`.
    fn update(*_, arm: usize, reward: f64): bool {
        if (arm >= _.num_arms || !isFiniteF64(reward)) {
            return false;
        }
        _.counts[arm] += 1;
        _.total_pulls += 1;
        const cnt_f: f64 = _.counts[arm] as f64;
        _.means[arm] += (reward - _.means[arm]) / cnt_f;
        return true;
    }

    /// Returns the arm with the highest empirical mean reward.
    fn bestArm(): usize? {
        if (_.num_arms == 0) {
            return null;
        }
        let best_idx: usize = 0;
        let best_mean: f64 = _.means[0];
        let has_pulled: bool = _.counts[0] > 0;
        let i: usize = 1;
        while (i < _.num_arms) {
            if (_.counts[i] > 0) {
                if (!has_pulled || _.means[i] > best_mean) {
                    best_mean = _.means[i];
                    best_idx = i;
                    has_pulled = true;
                }
            }
            i += 1;
        }
        return best_idx;
    }
}

/// Epsilon-Greedy multi-armed bandit supporting fixed and inverse-time decaying exploration.
pub struct EpsilonGreedyBandit {
    num_arms: usize,
    counts: []u64,
    means: []f64,
    total_pulls: u64,
    epsilon: f64,
    decay: f64,
    rng_state: u64,

    /// Initializes an Epsilon-Greedy bandit with `0.0 <= epsilon <= 1.0`.
    static fn init(counts: []u64, means: []f64, epsilon: f64, seed: u64): EpsilonGreedyBandit? {
        const k: usize = counts.len;
        if (k == 0 || means.len < k || !isFiniteF64(epsilon) || epsilon < 0.0 || epsilon > 1.0) {
            return null;
        }
        let i: usize = 0;
        while (i < k) {
            counts[i] = 0;
            means[i] = 0.0;
            i += 1;
        }
        return EpsilonGreedyBandit {
            num_arms: k,
            counts: counts,
            means: means,
            total_pulls: 0,
            epsilon: epsilon,
            decay: 0.0,
            rng_state: seed,
        };
    }

    /// Configures optional inverse-time exploration decay `eps_t = epsilon / (1 + decay * t)`.
    fn setDecay(*_, decay: f64): bool {
        if (!isFiniteF64(decay) || decay < 0.0) {
            return false;
        }
        _.decay = decay;
        return true;
    }

    /// Selects an arm via epsilon-greedy exploration/exploitation.
    fn selectArm(*_): usize? {
        if (_.num_arms == 0) {
            return null;
        }
        const eff_eps: f64 = _.epsilon / (1.0 + _.decay * (_.total_pulls as f64));
        const us: RngSample = nextUniform01(_.rng_state);
        _.rng_state = us.next_state;
        if (us.val < eff_eps) {
            const ns: u64 = _.rng_state +% SPLITMIX_INC;
            _.rng_state = ns;
            return (splitmixMix(ns) % (_.num_arms as u64)) as usize;
        }
        return _.bestArm();
    }

    /// Updates empirical mean and pull count for `arm` with observed `reward`.
    fn update(*_, arm: usize, reward: f64): bool {
        if (arm >= _.num_arms || !isFiniteF64(reward)) {
            return false;
        }
        _.counts[arm] += 1;
        _.total_pulls += 1;
        const cnt_f: f64 = _.counts[arm] as f64;
        _.means[arm] += (reward - _.means[arm]) / cnt_f;
        return true;
    }

    /// Returns the arm with the highest empirical mean reward.
    fn bestArm(): usize? {
        if (_.num_arms == 0) {
            return null;
        }
        let best_idx: usize = 0;
        let best_mean: f64 = _.means[0];
        let i: usize = 1;
        while (i < _.num_arms) {
            if (_.means[i] > best_mean) {
                best_mean = _.means[i];
                best_idx = i;
            }
            i += 1;
        }
        return best_idx;
    }
}

/// Beta-Bernoulli Thompson Sampling bandit using exact Marsaglia-Tsang Beta posterior draws.
pub struct ThompsonBandit {
    num_arms: usize,
    alphas: []f64,
    betas: []f64,
    rng_state: u64,

    /// Initializes uniform `Beta(1, 1)` priors across caller-owned `alphas` and `betas`.
    static fn init(alphas: []f64, betas: []f64, seed: u64): ThompsonBandit? {
        const k: usize = alphas.len;
        if (k == 0 || betas.len < k) {
            return null;
        }
        let i: usize = 0;
        while (i < k) {
            alphas[i] = 1.0;
            betas[i] = 1.0;
            i += 1;
        }
        return ThompsonBandit {
            num_arms: k,
            alphas: alphas,
            betas: betas,
            rng_state: seed,
        };
    }

    /// Samples one posterior reward mean per arm from `Beta(alpha_i, beta_i)` and picks the maximum.
    fn selectArm(*_): usize? {
        if (_.num_arms == 0) {
            return null;
        }
        let best_arm: usize = 0;
        let best_sample: f64 = -1.0;
        let i: usize = 0;
        while (i < _.num_arms) {
            const draw: RngSample = sampleBeta(_.alphas[i], _.betas[i], _.rng_state);
            _.rng_state = draw.next_state;
            if (i == 0 || draw.val > best_sample) {
                best_sample = draw.val;
                best_arm = i;
            }
            i += 1;
        }
        return best_arm;
    }

    /// Updates Beta posterior parameters with `reward_01 in [0.0, 1.0]`.
    fn update(*_, arm: usize, reward_01: f64): bool {
        if (arm >= _.num_arms || !isFiniteF64(reward_01) || reward_01 < 0.0 || reward_01 > 1.0) {
            return false;
        }
        _.alphas[arm] += reward_01;
        _.betas[arm] += (1.0 - reward_01);
        return true;
    }

    /// Returns the arm with the highest posterior mean `alpha / (alpha + beta)`.
    fn bestArm(): usize? {
        if (_.num_arms == 0) {
            return null;
        }
        let best_arm: usize = 0;
        let best_mean: f64 = _.alphas[0] / (_.alphas[0] + _.betas[0]);
        let i: usize = 1;
        while (i < _.num_arms) {
            const mean_i: f64 = _.alphas[i] / (_.alphas[i] + _.betas[i]);
            if (mean_i > best_mean) {
                best_mean = mean_i;
                best_arm = i;
            }
            i += 1;
        }
        return best_arm;
    }
}

/// EXP3 Adversarial Multi-Armed Bandit with log-weight shift stabilization (overflow-free).
pub struct Exp3Bandit {
    num_arms: usize,
    log_weights: []f64,
    probs: []f64,
    gamma: f64,
    rng_state: u64,

    /// Initializes EXP3 with exploration parameter `0 < gamma <= 1`.
    static fn init(log_weights: []f64, probs: []f64, gamma: f64, seed: u64): Exp3Bandit? {
        const k: usize = log_weights.len;
        if (k == 0 || probs.len < k || !isFiniteF64(gamma) || gamma <= 0.0 || gamma > 1.0) {
            return null;
        }
        const uniform: f64 = 1.0 / (k as f64);
        let i: usize = 0;
        while (i < k) {
            log_weights[i] = 0.0;
            probs[i] = uniform;
            i += 1;
        }
        return Exp3Bandit {
            num_arms: k,
            log_weights: log_weights,
            probs: probs,
            gamma: gamma,
            rng_state: seed,
        };
    }

    fn recomputeProbs(*_) {
        const k: usize = _.num_arms;
        let max_lw: f64 = _.log_weights[0];
        let i: usize = 1;
        while (i < k) {
            if (_.log_weights[i] > max_lw) {
                max_lw = _.log_weights[i];
            }
            i += 1;
        }

        // Shift log-weights so max == 0.0 to prevent exp overflow over arbitrarily many rounds
        let sum_exp: f64 = 0.0;
        let j: usize = 0;
        while (j < k) {
            _.log_weights[j] -= max_lw;
            const w: f64 = std.math.exp(_.log_weights[j]);
            _.probs[j] = w;
            sum_exp += w;
            j += 1;
        }

        const inv_sum: f64 = 1.0 / sum_exp;
        const mix_uniform: f64 = _.gamma / (k as f64);
        const keep: f64 = 1.0 - _.gamma;
        let p_sum: f64 = 0.0;
        let m: usize = 0;
        while (m < k) {
            const p: f64 = keep * (_.probs[m] * inv_sum) + mix_uniform;
            _.probs[m] = p;
            p_sum += p;
            m += 1;
        }
        if (p_sum > 0.0) {
            const inv_p: f64 = 1.0 / p_sum;
            let n_idx: usize = 0;
            while (n_idx < k) {
                _.probs[n_idx] *= inv_p;
                n_idx += 1;
            }
        }
    }

    /// Samples an arm from the current EXP3 probability distribution.
    fn selectArm(*_): usize? {
        if (_.num_arms == 0) {
            return null;
        }
        const us: RngSample = nextUniform01(_.rng_state);
        _.rng_state = us.next_state;
        let cum: f64 = 0.0;
        let i: usize = 0;
        while (i + 1 < _.num_arms) {
            cum += _.probs[i];
            if (us.val < cum) {
                return i;
            }
            i += 1;
        }
        return _.num_arms - 1;
    }

    /// Updates log-weights with importance-weighted reward `reward_01 in [0.0, 1.0]`.
    fn update(*_, arm: usize, reward_01: f64): bool {
        if (arm >= _.num_arms || !isFiniteF64(reward_01) || reward_01 < 0.0 || reward_01 > 1.0) {
            return false;
        }
        const p_arm: f64 = _.probs[arm];
        if (p_arm <= 0.0 || !isFiniteF64(p_arm)) {
            return false;
        }
        const est_reward: f64 = reward_01 / p_arm;
        _.log_weights[arm] += (_.gamma / (_.num_arms as f64)) * est_reward;
        _.recomputeProbs();
        return true;
    }

    /// Returns a read-only view of the current normalized arm selection probabilities.
    fn probabilities(): []const f64 {
        return _.probs[0.._.num_arms];
    }
}

/// Computes cumulative expected regret `sum_t (best_mean - chosen_means[t])`.
pub fn cumulativeRegret(best_mean: f64, chosen_means: []const f64): f64 {
    let regret: f64 = 0.0;
    let i: usize = 0;
    while (i < chosen_means.len) {
        regret += best_mean - chosen_means[i];
        i += 1;
    }
    return regret;
}
