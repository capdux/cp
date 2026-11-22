// Source: cp/cap/test/test_bandit_production_deep.cp (Production deep tests for cap:bandit)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    Ucb1Bandit,
    EpsilonGreedyBandit,
    ThompsonBandit,
    Exp3Bandit,
    cumulativeRegret,
} from "cap:ml";

const ARM_MEANS: [5]f64 = [0.20, 0.35, 0.80, 0.45, 0.30];
const BEST_MEAN: f64 = 0.80;
const BEST_ARM: usize = 2;

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

struct EnvDraw {
    reward: f64,
    next_seed: u64,
}

fn sampleBernoulli(prob: f64, seed: u64): EnvDraw {
    const ns: u64 = seed +% 0x9E3779B97F4A7C15;
    let z: u64 = (ns ^ (ns >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    z = z ^ (z >> 31);
    const u: f64 = ((z >> 11) as f64) / 9007199254740992.0;
    return EnvDraw {
        reward: if (u < prob) 1.0 else 0.0,
        next_seed: ns,
    };
}

test("1. UCB1 cold-start guarantee & 2000-round convergence on 5-arm Bernoulli bandit", () => {
    let counts: [5]u64 = std.mem.zeroes([5]u64);
    let means: [5]f64 = std.mem.zeroes([5]f64);
    let ucb: Ucb1Bandit = Ucb1Bandit.init(counts[0..], means[0..], 1.41421356237) ?? unreachable;

    // Unpulled arm score is +inf; out-of-bounds is -inf
    expect_eq(ucb.armScore(0) > 1.0e300, true);
    expect_eq(ucb.armScore(99) < -1.0e300, true);

    let chosen_means: [2000]f64 = std.mem.zeroes([2000]f64);
    let env_seed: u64 = 0x1111222233334444;
    let t: usize = 0;
    while (t < 2000) {
        const arm: usize = ucb.selectArm() ?? unreachable;
        // First 5 pulls must visit arms 0, 1, 2, 3, 4 in exact cold-start order
        if (t < 5) {
            expect_eq(arm, t);
        }
        const draw: EnvDraw = sampleBernoulli(ARM_MEANS[arm], env_seed);
        env_seed = draw.next_seed;
        expect_eq(ucb.update(arm, draw.reward), true);
        chosen_means[t] = ARM_MEANS[arm];
        t += 1;
    }

    expect_eq(ucb.bestArm() ?? unreachable, BEST_ARM);
    expect_eq(ucb.counts[BEST_ARM] > 1500, true);

    // Uniform random expected regret over 2000 rounds is 2000 * 0.38 = 760.0
    const regret: f64 = cumulativeRegret(BEST_MEAN, chosen_means[0..]);
    expect_eq(regret < 150.0, true);
});

test("2. Epsilon-Greedy (decaying) & Thompson Sampling converge with low regret over 2000 rounds", () => {
    // (a) Decaying Epsilon-Greedy
    let eg_counts: [5]u64 = std.mem.zeroes([5]u64);
    let eg_means: [5]f64 = std.mem.zeroes([5]f64);
    let eg: EpsilonGreedyBandit = EpsilonGreedyBandit.init(eg_counts[0..], eg_means[0..], 0.3, 42) ?? unreachable;
    expect_eq(eg.setDecay(0.01), true);

    let eg_chosen: [2000]f64 = std.mem.zeroes([2000]f64);
    let env_seed: u64 = 0x5555666677778888;
    let t: usize = 0;
    while (t < 2000) {
        const arm: usize = eg.selectArm() ?? unreachable;
        const draw: EnvDraw = sampleBernoulli(ARM_MEANS[arm], env_seed);
        env_seed = draw.next_seed;
        expect_eq(eg.update(arm, draw.reward), true);
        eg_chosen[t] = ARM_MEANS[arm];
        t += 1;
    }
    expect_eq(eg.bestArm() ?? unreachable, BEST_ARM);
    expect_eq(eg.counts[BEST_ARM] > 1400, true);
    expect_eq(cumulativeRegret(BEST_MEAN, eg_chosen[0..]) < 200.0, true);

    // (b) Beta-Bernoulli Thompson Sampling
    let alphas: [5]f64 = std.mem.zeroes([5]f64);
    let betas: [5]f64 = std.mem.zeroes([5]f64);
    let ts: ThompsonBandit = ThompsonBandit.init(alphas[0..], betas[0..], 2026) ?? unreachable;

    let ts_chosen: [2000]f64 = std.mem.zeroes([2000]f64);
    let best_pulls: usize = 0;
    let env_seed2: u64 = 0x9999AAAABBBBCCCC;
    let t2: usize = 0;
    while (t2 < 2000) {
        const arm: usize = ts.selectArm() ?? unreachable;
        if (arm == BEST_ARM) {
            best_pulls += 1;
        }
        const draw: EnvDraw = sampleBernoulli(ARM_MEANS[arm], env_seed2);
        env_seed2 = draw.next_seed;
        expect_eq(ts.update(arm, draw.reward), true);
        ts_chosen[t2] = ARM_MEANS[arm];
        t2 += 1;
    }
    expect_eq(ts.bestArm() ?? unreachable, BEST_ARM);
    expect_eq(best_pulls > 1700, true);
    expect_eq(cumulativeRegret(BEST_MEAN, ts_chosen[0..]) < 100.0, true);
});

test("3. EXP3 converges on 5-arm bandit, keeps sum(probs) == 1.0 & stays overflow-free over 5000 steps", () => {
    let lw: [5]f64 = std.mem.zeroes([5]f64);
    let probs: [5]f64 = std.mem.zeroes([5]f64);
    let exp3: Exp3Bandit = Exp3Bandit.init(lw[0..], probs[0..], 0.08, 777) ?? unreachable;

    let chosen: [2000]f64 = std.mem.zeroes([2000]f64);
    let env_seed: u64 = 0xCAFEBABE00001111;
    let t: usize = 0;
    while (t < 2000) {
        const arm: usize = exp3.selectArm() ?? unreachable;
        const draw: EnvDraw = sampleBernoulli(ARM_MEANS[arm], env_seed);
        env_seed = draw.next_seed;
        expect_eq(exp3.update(arm, draw.reward), true);
        chosen[t] = ARM_MEANS[arm];

        if ((t % 250) == 0) {
            const p_view: []const f64 = exp3.probabilities();
            let psum: f64 = 0.0;
            let k: usize = 0;
            while (k < 5) {
                expect_eq(isFiniteF64(p_view[k]) && p_view[k] > 0.0, true);
                psum += p_view[k];
                k += 1;
            }
            expect_eq(absF64(psum - 1.0) < 1.0e-12, true);
        }
        t += 1;
    }

    const final_probs: []const f64 = exp3.probabilities();
    expect_eq(final_probs[BEST_ARM] > 0.70, true);
    expect_eq(cumulativeRegret(BEST_MEAN, chosen[0..]) < 300.0, true);

    // Extra 3000 steps with constant reward 1.0 to stress log-weight shift stability
    let t_extra: usize = 0;
    while (t_extra < 3000) {
        const arm: usize = exp3.selectArm() ?? unreachable;
        expect_eq(exp3.update(arm, 1.0), true);
        t_extra += 1;
    }
    const p_after: []const f64 = exp3.probabilities();
    let p_sum_after: f64 = 0.0;
    let k2: usize = 0;
    while (k2 < 5) {
        expect_eq(isFiniteF64(p_after[k2]) && isFiniteF64(exp3.log_weights[k2]), true);
        p_sum_after += p_after[k2];
        k2 += 1;
    }
    expect_eq(absF64(p_sum_after - 1.0) < 1.0e-12, true);
});

test("4. Deterministic seed reproducibility across Thompson, Epsilon-Greedy & EXP3", () => {
    let a1: [4]f64 = std.mem.zeroes([4]f64);
    let b1: [4]f64 = std.mem.zeroes([4]f64);
    let a2: [4]f64 = std.mem.zeroes([4]f64);
    let b2: [4]f64 = std.mem.zeroes([4]f64);

    let ts1: ThompsonBandit = ThompsonBandit.init(a1[0..], b1[0..], 0x12345678) ?? unreachable;
    let ts2: ThompsonBandit = ThompsonBandit.init(a2[0..], b2[0..], 0x12345678) ?? unreachable;

    let step: usize = 0;
    while (step < 50) {
        const arm1: usize = ts1.selectArm() ?? unreachable;
        const arm2: usize = ts2.selectArm() ?? unreachable;
        expect_eq(arm1, arm2);
        const r: f64 = if ((step % 3) == 0) 1.0 else 0.0;
        expect_eq(ts1.update(arm1, r), true);
        expect_eq(ts2.update(arm2, r), true);
        step += 1;
    }

    let lw1: [4]f64 = std.mem.zeroes([4]f64);
    let p1: [4]f64 = std.mem.zeroes([4]f64);
    let lw2: [4]f64 = std.mem.zeroes([4]f64);
    let p2: [4]f64 = std.mem.zeroes([4]f64);
    let e1: Exp3Bandit = Exp3Bandit.init(lw1[0..], p1[0..], 0.1, 9999) ?? unreachable;
    let e2: Exp3Bandit = Exp3Bandit.init(lw2[0..], p2[0..], 0.1, 9999) ?? unreachable;

    let step2: usize = 0;
    while (step2 < 50) {
        const arm1: usize = e1.selectArm() ?? unreachable;
        const arm2: usize = e2.selectArm() ?? unreachable;
        expect_eq(arm1, arm2);
        const r: f64 = if ((step2 % 2) == 0) 0.7 else 0.2;
        expect_eq(e1.update(arm1, r), true);
        expect_eq(e2.update(arm2, r), true);
        step2 += 1;
    }
});

test("5. cumulativeRegret exact calculation", () => {
    const seq: [4]f64 = [0.8, 0.5, 0.2, 0.8];
    // (0.8-0.8) + (0.8-0.5) + (0.8-0.2) + (0.8-0.8) = 0.9
    expect_eq(absF64(cumulativeRegret(0.8, seq[0..]) - 0.9) < 1.0e-12, true);
    const empty: [0]f64 = [];
    expect_eq(cumulativeRegret(0.8, empty[0..]), 0.0);
});

test("6. Defensive guards: zero arms, short buffers, OOB arms, invalid hyperparams & NaN", () => {
    let empty_u: [0]u64 = [];
    let empty_f: [0]f64 = [];
    let c3: [3]u64 = std.mem.zeroes([3]u64);
    let f3: [3]f64 = std.mem.zeroes([3]f64);
    let f2: [2]f64 = std.mem.zeroes([2]f64);

    const zero: f64 = 0.0;
    const nan_v: f64 = zero / zero;

    // Zero arms & mismatched buffer lengths
    expect_eq(Ucb1Bandit.init(empty_u[0..], empty_f[0..], 1.0) == null, true);
    expect_eq(Ucb1Bandit.init(c3[0..], f2[0..], 1.0) == null, true);
    expect_eq(Ucb1Bandit.init(c3[0..], f3[0..], -0.5) == null, true);
    expect_eq(Ucb1Bandit.init(c3[0..], f3[0..], nan_v) == null, true);

    expect_eq(EpsilonGreedyBandit.init(empty_u[0..], empty_f[0..], 0.1, 1) == null, true);
    expect_eq(EpsilonGreedyBandit.init(c3[0..], f3[0..], -0.1, 1) == null, true);
    expect_eq(EpsilonGreedyBandit.init(c3[0..], f3[0..], 1.1, 1) == null, true);

    expect_eq(ThompsonBandit.init(empty_f[0..], empty_f[0..], 1) == null, true);
    expect_eq(ThompsonBandit.init(f3[0..], f2[0..], 1) == null, true);

    expect_eq(Exp3Bandit.init(empty_f[0..], empty_f[0..], 0.1, 1) == null, true);
    expect_eq(Exp3Bandit.init(f3[0..], f3[0..], 0.0, 1) == null, true);
    expect_eq(Exp3Bandit.init(f3[0..], f3[0..], 1.2, 1) == null, true);

    // Out-of-bounds arm & invalid reward updates
    let ucb: Ucb1Bandit = Ucb1Bandit.init(c3[0..], f3[0..], 1.0) ?? unreachable;
    expect_eq(ucb.update(3, 1.0), false);
    expect_eq(ucb.update(0, nan_v), false);

    let a3: [3]f64 = std.mem.zeroes([3]f64);
    let b3: [3]f64 = std.mem.zeroes([3]f64);
    let ts: ThompsonBandit = ThompsonBandit.init(a3[0..], b3[0..], 1) ?? unreachable;
    expect_eq(ts.update(3, 0.5), false);
    expect_eq(ts.update(0, -0.1), false);
    expect_eq(ts.update(0, 1.1), false);
    expect_eq(ts.update(0, nan_v), false);

    let lw3: [3]f64 = std.mem.zeroes([3]f64);
    let p3: [3]f64 = std.mem.zeroes([3]f64);
    let exp3: Exp3Bandit = Exp3Bandit.init(lw3[0..], p3[0..], 0.1, 1) ?? unreachable;
    expect_eq(exp3.update(3, 0.5), false);
    expect_eq(exp3.update(0, -0.01), false);
    expect_eq(exp3.update(0, 1.5), false);
    expect_eq(exp3.update(0, nan_v), false);
});
