// Origin: Pure-CP Stdlib (Zero-Alloc Genetic Algorithms: Real-Coded GA & Permutation GA with Order Crossover) | Created: 2026-11-22

const TWO_PI: f64 = 6.28318530717958647692;
const BLX_ALPHA: f64 = 0.3;

/// Continuous objective function to minimize: `f(x) -> cost`.
pub type RealObjectiveFn = fn(x: []const f64): f64;

/// Discrete permutation objective function to minimize: `f(perm) -> cost`.
pub type PermObjectiveFn = fn(perm: []const usize): f64;

/// Hyperparameters for real-coded genetic optimization (`realGaMinimize`).
pub struct RealGaConfig {
    generations: usize,
    crossover_rate: f64,
    mutation_rate: f64,
    mutation_sigma: f64,
    tournament_k: usize,
    elitism_count: usize,
    seed: u64,

    /// Default real-coded GA configuration.
    static fn default(): RealGaConfig {
        return RealGaConfig {
            generations: 250,
            crossover_rate: 0.85,
            mutation_rate: 0.2,
            mutation_sigma: 0.15,
            tournament_k: 3,
            elitism_count: 2,
            seed: 42,
        };
    }
}

/// Hyperparameters for permutation genetic optimization (`permGaMinimize`).
pub struct PermGaConfig {
    generations: usize,
    crossover_rate: f64,
    mutation_rate: f64,
    tournament_k: usize,
    elitism_count: usize,
    seed: u64,

    /// Default permutation GA configuration.
    static fn default(): PermGaConfig {
        return PermGaConfig {
            generations: 300,
            crossover_rate: 0.85,
            mutation_rate: 0.25,
            tournament_k: 3,
            elitism_count: 2,
            seed: 42,
        };
    }
}

/// Summary returned by `realGaMinimize` and `permGaMinimize`.
pub struct GaResult {
    best_value: f64,
    generations: usize,
    evals: usize,
}

/// Caller-provided zero-allocation double-buffered workspace for `realGaMinimize`.
/// Required lengths: `pop_a >= pop_size * dim`, `pop_b >= pop_size * dim`,
/// `fitness >= pop_size`, `order >= pop_size`.
pub struct RealGaWorkspace {
    pop_a: []f64,
    pop_b: []f64,
    fitness: []f64,
    order: []usize,
}

/// Caller-provided zero-allocation double-buffered workspace for `permGaMinimize`.
/// Required lengths: `pop_a >= pop_size * n_genes`, `pop_b >= pop_size * n_genes`,
/// `fitness >= pop_size`, `order >= pop_size`, `used_mark >= n_genes`.
pub struct PermGaWorkspace {
    pop_a: []usize,
    pop_b: []usize,
    fitness: []f64,
    order: []usize,
    used_mark: []u8,
}

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

fn checkMatrixBounds(buf_len: usize, rows: usize, cols: usize): bool {
    return rows > 0 && cols > 0 && (buf_len / cols) >= rows;
}

fn splitMixNext(state: *u64): u64 {
    state.* = state.* +% 0x9E3779B97F4A7C15;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn randUnit(state: *u64): f64 {
    const bits: u64 = splitMixNext(state) >> 11;
    return (bits as f64) * (1.0 / 9007199254740992.0);
}

fn randIndex(state: *u64, bound: usize): usize {
    return (splitMixNext(state) as usize) % bound;
}

fn randNormal(state: *u64): f64 {
    let r1: f64 = randUnit(state);
    if (r1 < 1.0e-15) {
        r1 = 1.0e-15;
    }
    const r2: f64 = randUnit(state);
    return @sqrt(-2.0 * @log(r1)) * @cos(TWO_PI * r2);
}

fn sortOrderByFitness(order: []usize, fitness: []const f64, pop_size: usize) {
    let i: usize = 0;
    while (i < pop_size) {
        order[i] = i;
        i += 1;
    }
    // In-place Shell sort over `order[0..pop_size]` by ascending `fitness`.
    let gap: usize = pop_size / 2;
    while (gap > 0) {
        let idx: usize = gap;
        while (idx < pop_size) {
            const key_id: usize = order[idx];
            const key_fit: f64 = fitness[key_id];
            let j: usize = idx;
            while (j >= gap) {
                const prev_id: usize = order[j - gap];
                if (fitness[prev_id] <= key_fit) {
                    break;
                }
                order[j] = prev_id;
                j -= gap;
            }
            order[j] = key_id;
            idx += 1;
        }
        gap = gap / 2;
    }
}

fn tournamentSelect(fitness: []const f64, pop_size: usize, k: usize, rng: *u64): usize {
    let best: usize = randIndex(rng, pop_size);
    let best_fit: f64 = fitness[best];
    let t: usize = 1;
    while (t < k) {
        const cand: usize = randIndex(rng, pop_size);
        const cand_fit: f64 = fitness[cand];
        if (cand_fit < best_fit) {
            best = cand;
            best_fit = cand_fit;
        }
        t += 1;
    }
    return best;
}

/// Validates that `perm[0..perm.len]` is a bijection over `0..perm.len - 1`.
pub fn isValidPermutation(perm: []const usize, seen_scratch: *[]u8): bool {
    const n: usize = perm.len;
    if (n == 0 || seen_scratch.*.len < n) {
        return false;
    }
    let seen: []u8 = seen_scratch.*;
    let i: usize = 0;
    while (i < n) {
        seen[i] = 0;
        i += 1;
    }
    let j: usize = 0;
    while (j < n) {
        const g: usize = perm[j];
        if (g >= n || seen[g] != 0) {
            return false;
        }
        seen[g] = 1;
        j += 1;
    }
    return true;
}

/// Order Crossover (OX1) operator producing a guaranteed valid permutation of `0..n-1`.
/// Copies `p1[min(cut1, cut2)..=max(cut1, cut2)]` directly and fills remaining slots
/// in cyclic order from `p2` starting after the second cut point.
pub fn orderCrossover(
    p1: []const usize,
    p2: []const usize,
    cut1: usize,
    cut2: usize,
    child_out: *[]usize,
    used_mark: *[]u8,
): bool {
    const n: usize = p1.len;
    if (n == 0 || p2.len != n || child_out.*.len < n || used_mark.*.len < n) {
        return false;
    }
    if (cut1 >= n || cut2 >= n) {
        return false;
    }
    if (!isValidPermutation(p1, used_mark) || !isValidPermutation(p2, used_mark)) {
        return false;
    }

    const c1: usize = if (cut1 <= cut2) cut1 else cut2;
    const c2: usize = if (cut1 <= cut2) cut2 else cut1;

    let child: []usize = child_out.*;
    let used: []u8 = used_mark.*;

    let k: usize = 0;
    while (k < n) {
        used[k] = 0;
        k += 1;
    }

    let i: usize = c1;
    while (i <= c2) {
        const gene: usize = p1[i];
        child[i] = gene;
        used[gene] = 1;
        i += 1;
    }

    let write_pos: usize = (c2 + 1) % n;
    let step: usize = 0;
    while (step < n) {
        const read_pos: usize = (c2 + 1 + step) % n;
        const gene: usize = p2[read_pos];
        if (used[gene] == 0) {
            child[write_pos] = gene;
            used[gene] = 1;
            write_pos = (write_pos + 1) % n;
        }
        step += 1;
    }

    return true;
}

/// Minimizes a continuous box-constrained objective using a zero-allocation Real-Coded GA
/// (tournament selection, BLX-alpha crossover, Gaussian mutation with annealing, elitism, and box clamping).
pub fn realGaMinimize(
    obj: RealObjectiveFn,
    lower: []const f64,
    upper: []const f64,
    pop_size: usize,
    cfg: RealGaConfig,
    best_x_out: *[]f64,
    ws: *RealGaWorkspace,
): GaResult? {
    const dim: usize = lower.len;
    if (dim == 0 || upper.len < dim || best_x_out.*.len < dim) {
        return null;
    }
    if (pop_size < 2 || cfg.tournament_k == 0 || cfg.tournament_k > pop_size) {
        return null;
    }
    if (cfg.elitism_count >= pop_size) {
        return null;
    }
    if (
        !isFiniteF64(cfg.crossover_rate) || cfg.crossover_rate < 0.0 || cfg.crossover_rate > 1.0 ||
        !isFiniteF64(cfg.mutation_rate) || cfg.mutation_rate < 0.0 || cfg.mutation_rate > 1.0 ||
        !isFiniteF64(cfg.mutation_sigma) || cfg.mutation_sigma < 0.0
    ) {
        return null;
    }
    if (
        !checkMatrixBounds(ws.*.pop_a.len, pop_size, dim) ||
        !checkMatrixBounds(ws.*.pop_b.len, pop_size, dim) ||
        ws.*.fitness.len < pop_size ||
        ws.*.order.len < pop_size
    ) {
        return null;
    }

    let d_chk: usize = 0;
    while (d_chk < dim) {
        if (!isFiniteF64(lower[d_chk]) || !isFiniteF64(upper[d_chk]) || lower[d_chk] >= upper[d_chk]) {
            return null;
        }
        d_chk += 1;
    }

    let rng: u64 = cfg.seed;
    let cur_pop: []f64 = ws.*.pop_a;
    let next_pop: []f64 = ws.*.pop_b;
    let fitness: []f64 = ws.*.fitness;
    let order: []usize = ws.*.order;
    let best_x: []f64 = best_x_out.*;

    // Initialize population uniformly within [lower, upper].
    let evals: usize = 0;
    let i: usize = 0;
    while (i < pop_size) {
        const row: usize = i * dim;
        let j: usize = 0;
        while (j < dim) {
            cur_pop[row + j] = lower[j] + randUnit(*rng) * (upper[j] - lower[j]);
            j += 1;
        }
        const fval: f64 = obj(cur_pop[row..(row + dim)]);
        evals += 1;
        if (!isFiniteF64(fval)) {
            return null;
        }
        fitness[i] = fval;
        i += 1;
    }

    sortOrderByFitness(order, fitness, pop_size);
    let best_val: f64 = fitness[order[0]];
    let b0: usize = 0;
    while (b0 < dim) {
        best_x[b0] = cur_pop[order[0] * dim + b0];
        b0 += 1;
    }

    let gen: usize = 0;
    while (gen < cfg.generations) {
        // Anneal mutation standard deviation smoothly over generations for fine convergence.
        const progress: f64 = if (cfg.generations > 1) {
            (gen as f64) / ((cfg.generations - 1) as f64)
        } else {
            0.0
        };
        const sigma_eff: f64 = cfg.mutation_sigma * (1.0 - (0.85 * progress));

        // Copy elites verbatim to preserve monotonic best fitness.
        let e: usize = 0;
        while (e < cfg.elitism_count) {
            const src_row: usize = order[e] * dim;
            const dst_row: usize = e * dim;
            let j: usize = 0;
            while (j < dim) {
                next_pop[dst_row + j] = cur_pop[src_row + j];
                j += 1;
            }
            e += 1;
        }

        // Produce remaining offspring via tournament selection + BLX-alpha + Gaussian mutation.
        let child: usize = cfg.elitism_count;
        while (child < pop_size) {
            const p1: usize = tournamentSelect(fitness, pop_size, cfg.tournament_k, *rng);
            const p2: usize = tournamentSelect(fitness, pop_size, cfg.tournament_k, *rng);
            const do_cx: bool = randUnit(*rng) < cfg.crossover_rate;

            const r1: usize = p1 * dim;
            const r2: usize = p2 * dim;
            const dst: usize = child * dim;

            let j: usize = 0;
            while (j < dim) {
                let gene: f64 = cur_pop[r1 + j];
                if (do_cx) {
                    const g1: f64 = cur_pop[r1 + j];
                    const g2: f64 = cur_pop[r2 + j];
                    const lo_v: f64 = if (g1 <= g2) g1 else g2;
                    const hi_v: f64 = if (g1 <= g2) g2 else g1;
                    const span: f64 = hi_v - lo_v;
                    const ext_lo: f64 = lo_v - (BLX_ALPHA * span);
                    const ext_hi: f64 = hi_v + (BLX_ALPHA * span);
                    gene = ext_lo + randUnit(*rng) * (ext_hi - ext_lo);
                }
                if (sigma_eff > 0.0 && randUnit(*rng) < cfg.mutation_rate) {
                    gene += randNormal(*rng) * sigma_eff;
                }
                if (gene < lower[j]) {
                    gene = lower[j];
                } else if (gene > upper[j]) {
                    gene = upper[j];
                }
                next_pop[dst + j] = gene;
                j += 1;
            }
            child += 1;
        }

        // Swap double buffers without copying.
        const tmp_pop: []f64 = cur_pop;
        cur_pop = next_pop;
        next_pop = tmp_pop;

        // Evaluate new generation.
        let k: usize = 0;
        while (k < pop_size) {
            const row: usize = k * dim;
            const fval: f64 = obj(cur_pop[row..(row + dim)]);
            evals += 1;
            if (!isFiniteF64(fval)) {
                return null;
            }
            fitness[k] = fval;
            k += 1;
        }

        sortOrderByFitness(order, fitness, pop_size);
        if (fitness[order[0]] < best_val) {
            best_val = fitness[order[0]];
            let bj: usize = 0;
            while (bj < dim) {
                best_x[bj] = cur_pop[order[0] * dim + bj];
                bj += 1;
            }
        }

        gen += 1;
    }

    return GaResult {
        best_value: best_val,
        generations: cfg.generations,
        evals: evals,
    };
}

/// Minimizes a discrete permutation objective using a zero-allocation Permutation GA
/// (tournament selection, Order Crossover OX1, inversion/swap mutation, and elitism).
pub fn permGaMinimize(
    obj: PermObjectiveFn,
    n_genes: usize,
    pop_size: usize,
    cfg: PermGaConfig,
    best_perm_out: *[]usize,
    ws: *PermGaWorkspace,
): GaResult? {
    if (n_genes == 0 || pop_size < 2 || best_perm_out.*.len < n_genes) {
        return null;
    }
    if (cfg.tournament_k == 0 || cfg.tournament_k > pop_size || cfg.elitism_count >= pop_size) {
        return null;
    }
    if (
        !isFiniteF64(cfg.crossover_rate) || cfg.crossover_rate < 0.0 || cfg.crossover_rate > 1.0 ||
        !isFiniteF64(cfg.mutation_rate) || cfg.mutation_rate < 0.0 || cfg.mutation_rate > 1.0
    ) {
        return null;
    }
    if (
        !checkMatrixBounds(ws.*.pop_a.len, pop_size, n_genes) ||
        !checkMatrixBounds(ws.*.pop_b.len, pop_size, n_genes) ||
        ws.*.fitness.len < pop_size ||
        ws.*.order.len < pop_size ||
        ws.*.used_mark.len < n_genes
    ) {
        return null;
    }

    let rng: u64 = cfg.seed;
    let cur_pop: []usize = ws.*.pop_a;
    let next_pop: []usize = ws.*.pop_b;
    let fitness: []f64 = ws.*.fitness;
    let order: []usize = ws.*.order;
    let used_mark: []u8 = ws.*.used_mark;
    let best_perm: []usize = best_perm_out.*;

    // Initialize population with Fisher-Yates shuffled permutations of 0..n_genes-1.
    let evals: usize = 0;
    let i: usize = 0;
    while (i < pop_size) {
        const row: usize = i * n_genes;
        let g: usize = 0;
        while (g < n_genes) {
            cur_pop[row + g] = g;
            g += 1;
        }
        if (n_genes > 1) {
            let k: usize = n_genes - 1;
            while (k > 0) {
                const swap_idx: usize = randIndex(*rng, k + 1);
                const tmp_g: usize = cur_pop[row + k];
                cur_pop[row + k] = cur_pop[row + swap_idx];
                cur_pop[row + swap_idx] = tmp_g;
                k -= 1;
            }
        }
        const fval: f64 = obj(cur_pop[row..(row + n_genes)]);
        evals += 1;
        if (!isFiniteF64(fval)) {
            return null;
        }
        fitness[i] = fval;
        i += 1;
    }

    sortOrderByFitness(order, fitness, pop_size);
    let best_val: f64 = fitness[order[0]];
    let b0: usize = 0;
    while (b0 < n_genes) {
        best_perm[b0] = cur_pop[order[0] * n_genes + b0];
        b0 += 1;
    }

    let gen: usize = 0;
    while (gen < cfg.generations) {
        // Copy elites verbatim.
        let e: usize = 0;
        while (e < cfg.elitism_count) {
            const src_row: usize = order[e] * n_genes;
            const dst_row: usize = e * n_genes;
            let j: usize = 0;
            while (j < n_genes) {
                next_pop[dst_row + j] = cur_pop[src_row + j];
                j += 1;
            }
            e += 1;
        }

        // Produce remaining offspring via tournament selection + OX1 + inversion/swap mutation.
        let child: usize = cfg.elitism_count;
        while (child < pop_size) {
            const p1: usize = tournamentSelect(fitness, pop_size, cfg.tournament_k, *rng);
            const p2: usize = tournamentSelect(fitness, pop_size, cfg.tournament_k, *rng);
            const r1: usize = p1 * n_genes;
            const r2: usize = p2 * n_genes;
            const dst: usize = child * n_genes;

            if (n_genes >= 2 && randUnit(*rng) < cfg.crossover_rate) {
                const cut1: usize = randIndex(*rng, n_genes);
                const cut2: usize = randIndex(*rng, n_genes);
                let child_slice: []usize = next_pop[dst..(dst + n_genes)];
                const ok: bool = orderCrossover(
                    cur_pop[r1..(r1 + n_genes)],
                    cur_pop[r2..(r2 + n_genes)],
                    cut1,
                    cut2,
                    *child_slice,
                    *used_mark,
                );
                if (!ok) {
                    return null;
                }
            } else {
                let j: usize = 0;
                while (j < n_genes) {
                    next_pop[dst + j] = cur_pop[r1 + j];
                    j += 1;
                }
            }

            if (n_genes >= 2 && randUnit(*rng) < cfg.mutation_rate) {
                let a: usize = randIndex(*rng, n_genes);
                let b: usize = randIndex(*rng, n_genes);
                if (a > b) {
                    const t_ab: usize = a;
                    a = b;
                    b = t_ab;
                }
                if (a < b) {
                    if (randUnit(*rng) < 0.75) {
                        // Inversion (2-opt segment reversal) mutation
                        let lo: usize = a;
                        let hi: usize = b;
                        while (lo < hi) {
                            const tmp_v: usize = next_pop[dst + lo];
                            next_pop[dst + lo] = next_pop[dst + hi];
                            next_pop[dst + hi] = tmp_v;
                            lo += 1;
                            hi -= 1;
                        }
                    } else {
                        // Swap mutation
                        const tmp_v: usize = next_pop[dst + a];
                        next_pop[dst + a] = next_pop[dst + b];
                        next_pop[dst + b] = tmp_v;
                    }
                }
            }

            child += 1;
        }

        // Swap double buffers without copying.
        const tmp_pop: []usize = cur_pop;
        cur_pop = next_pop;
        next_pop = tmp_pop;

        let k: usize = 0;
        while (k < pop_size) {
            const row: usize = k * n_genes;
            const fval: f64 = obj(cur_pop[row..(row + n_genes)]);
            evals += 1;
            if (!isFiniteF64(fval)) {
                return null;
            }
            fitness[k] = fval;
            k += 1;
        }

        sortOrderByFitness(order, fitness, pop_size);
        if (fitness[order[0]] < best_val) {
            best_val = fitness[order[0]];
            let bj: usize = 0;
            while (bj < n_genes) {
                best_perm[bj] = cur_pop[order[0] * n_genes + bj];
                bj += 1;
            }
        }

        gen += 1;
    }

    return GaResult {
        best_value: best_val,
        generations: cfg.generations,
        evals: evals,
    };
}
