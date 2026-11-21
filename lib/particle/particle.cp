// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Sequential Monte Carlo / Particle Filter: Systematic, Stratified & Multinomial Resampling)

const TWO_PI: f64 = 6.28318530717958647692;

/// Resampling strategy for Sequential Monte Carlo particle rejuvenation.
pub enum ResampleKind {
    Systematic,
    Stratified,
    Multinomial,
}

/// State transition callback: mutates `state_inout[0..dim]` in-place using control `u`, step `dt`, and `noise[0..dim]`.
pub type PropagateFn = fn(state_inout: *[]f64, u: []const f64, dt: f64, noise: []const f64);

/// Observation log-likelihood callback: evaluates `log p(z | state)`.
pub type LogLikelihoodFn = fn(state: []const f64, z: []const f64): f64;

fn negInfF64(): f64 {
    const z: f64 = 0.0;
    return -1.0 / z;
}

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

fn checkMatrixBounds(buf_len: usize, rows: usize, cols: usize): bool {
    return rows > 0 && cols > 0 && (buf_len / cols) >= rows;
}

fn checkFinitePrefix(data: []const f64, count: usize): bool {
    let i: usize = 0;
    while (i < count) {
        if (!isFiniteF64(data[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn splitMixMix(s: u64): u64 {
    let z: u64 = s;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn splitMixStep(s: u64): u64 {
    return s +% 0x9E3779B97F4A7C15;
}

fn unitFromState(s: u64): f64 {
    const bits: u64 = splitMixMix(s) >> 11;
    return (bits as f64) * (1.0 / 9007199254740992.0);
}

fn normalFromPair(s1: u64, s2: u64): f64 {
    let r1: f64 = unitFromState(s1);
    if (r1 < 1.0e-15) {
        r1 = 1.0e-15;
    }
    const r2: f64 = unitFromState(s2);
    return @sqrt(-2.0 * @log(r1)) * @cos(TWO_PI * r2);
}

/// Normalizes unnormalized log-weights via log-sum-exp to prevent underflow.
/// Writes normalized probabilities summing to `1.0` into `weights_out[0..log_weights.len]`
/// and returns the log marginal likelihood `log(sum(exp(log_weights)))`.
pub fn normalizeLogWeights(log_weights: []const f64, weights_out: *[]f64): f64? {
    const n: usize = log_weights.len;
    if (n == 0 || weights_out.*.len < n) {
        return null;
    }

    let has_finite: bool = false;
    let max_lw: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const lw: f64 = log_weights[i];
        // Reject NaN or +Inf; allow -Inf (zero likelihood particle).
        if (lw != lw || (!isFiniteF64(lw) && lw > 0.0)) {
            return null;
        }
        if (isFiniteF64(lw)) {
            if (!has_finite || lw > max_lw) {
                max_lw = lw;
                has_finite = true;
            }
        }
        i += 1;
    }
    if (!has_finite) {
        return null;
    }

    let dst: []f64 = weights_out.*;
    let sum_exp: f64 = 0.0;
    let j: usize = 0;
    while (j < n) {
        const lw: f64 = log_weights[j];
        if (isFiniteF64(lw)) {
            const e: f64 = @exp(lw - max_lw);
            dst[j] = e;
            sum_exp += e;
        } else {
            dst[j] = 0.0;
        }
        j += 1;
    }
    if (!isFiniteF64(sum_exp) || sum_exp <= 0.0) {
        return null;
    }

    const inv_sum: f64 = 1.0 / sum_exp;
    let k: usize = 0;
    while (k < n) {
        dst[k] = dst[k] * inv_sum;
        k += 1;
    }
    return max_lw + @log(sum_exp);
}

/// Computes Kish's Effective Sample Size `ESS = 1 / sum(w_i^2)` in `[1, N]`.
/// Returns `0.0` on empty or invalid weight slices.
pub fn effectiveSampleSize(weights: []const f64): f64 {
    if (weights.len == 0) {
        return 0.0;
    }
    let sum_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < weights.len) {
        const w: f64 = weights[i];
        if (!isFiniteF64(w) || w < 0.0) {
            return 0.0;
        }
        sum_sq += w * w;
        i += 1;
    }
    if (!isFiniteF64(sum_sq) || sum_sq <= 0.0) {
        return 0.0;
    }
    return 1.0 / sum_sq;
}

/// Generates `weights.len` ancestor particle indices in `0..weights.len - 1` using
/// Systematic (low-variance), Stratified, or Multinomial resampling.
pub fn resampleIndices(
    weights: []const f64,
    kind: ResampleKind,
    seed: u64,
    indices_out: *[]usize,
): bool {
    const n: usize = weights.len;
    if (n == 0 || indices_out.*.len < n) {
        return false;
    }

    let total_w: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const w: f64 = weights[i];
        if (!isFiniteF64(w) || w < 0.0) {
            return false;
        }
        total_w += w;
        i += 1;
    }
    if (!isFiniteF64(total_w) || total_w <= 0.0) {
        return false;
    }

    let out: []usize = indices_out.*;
    const inv_total: f64 = 1.0 / total_w;
    const inv_n: f64 = 1.0 / (n as f64);

    match (kind) {
        .Systematic => {
            const s1: u64 = splitMixStep(seed);
            const offset0: f64 = unitFromState(s1) * inv_n;
            let cdf: f64 = weights[0] * inv_total;
            let idx: usize = 0;
            let k: usize = 0;
            while (k < n) {
                const pos: f64 = offset0 + ((k as f64) * inv_n);
                while (pos > cdf && (idx + 1) < n) {
                    idx += 1;
                    cdf += weights[idx] * inv_total;
                }
                out[k] = idx;
                k += 1;
            }
        },
        .Stratified => {
            let r: u64 = seed;
            let cdf: f64 = weights[0] * inv_total;
            let idx: usize = 0;
            let k: usize = 0;
            while (k < n) {
                r = splitMixStep(r);
                const pos: f64 = ((k as f64) + unitFromState(r)) * inv_n;
                while (pos > cdf && (idx + 1) < n) {
                    idx += 1;
                    cdf += weights[idx] * inv_total;
                }
                out[k] = idx;
                k += 1;
            }
        },
        .Multinomial => {
            let r: u64 = seed;
            let k: usize = 0;
            while (k < n) {
                r = splitMixStep(r);
                const u: f64 = unitFromState(r) * total_w;
                let cdf: f64 = weights[0];
                let idx: usize = 0;
                while (u > cdf && (idx + 1) < n) {
                    idx += 1;
                    cdf += weights[idx];
                }
                out[k] = idx;
                k += 1;
            }
        },
    }

    return true;
}

/// Zero-allocation Sequential Monte Carlo (Particle Filter) backed by caller-owned buffers.
pub struct ParticleFilter {
    n_particles: usize,
    dim: usize,
    particles: []f64,
    weights: []f64,
    rng_state: u64,

    /// Initializes particles uniformly in the hyper-rectangle `[lower, upper]` with uniform weights `1/N`.
    static fn initUniform(
        particles_buf: *[]f64,
        weights_buf: *[]f64,
        n_particles: usize,
        lower: []const f64,
        upper: []const f64,
        seed: u64,
    ): ParticleFilter? {
        const dim: usize = lower.len;
        if (n_particles == 0 || dim == 0 || upper.len < dim) {
            return null;
        }
        if (!checkMatrixBounds(particles_buf.*.len, n_particles, dim) || weights_buf.*.len < n_particles) {
            return null;
        }
        let d: usize = 0;
        while (d < dim) {
            if (!isFiniteF64(lower[d]) || !isFiniteF64(upper[d]) || lower[d] > upper[d]) {
                return null;
            }
            d += 1;
        }

        let rng: u64 = seed;
        let pts: []f64 = particles_buf.*;
        let wts: []f64 = weights_buf.*;
        const init_w: f64 = 1.0 / (n_particles as f64);

        let i: usize = 0;
        while (i < n_particles) {
            const row: usize = i * dim;
            let j: usize = 0;
            while (j < dim) {
                rng = splitMixStep(rng);
                pts[row + j] = lower[j] + (unitFromState(rng) * (upper[j] - lower[j]));
                j += 1;
            }
            wts[i] = init_w;
            i += 1;
        }

        return ParticleFilter {
            n_particles: n_particles,
            dim: dim,
            particles: pts,
            weights: wts,
            rng_state: rng,
        };
    }

    /// Initializes particles from an isotropic/diagonal Gaussian `N(mean, diag(std_dev^2))` with uniform weights `1/N`.
    static fn initGaussian(
        particles_buf: *[]f64,
        weights_buf: *[]f64,
        n_particles: usize,
        mean: []const f64,
        std_dev: []const f64,
        seed: u64,
    ): ParticleFilter? {
        const dim: usize = mean.len;
        if (n_particles == 0 || dim == 0 || std_dev.len < dim) {
            return null;
        }
        if (!checkMatrixBounds(particles_buf.*.len, n_particles, dim) || weights_buf.*.len < n_particles) {
            return null;
        }
        let d: usize = 0;
        while (d < dim) {
            if (!isFiniteF64(mean[d]) || !isFiniteF64(std_dev[d]) || std_dev[d] < 0.0) {
                return null;
            }
            d += 1;
        }

        let rng: u64 = seed;
        let pts: []f64 = particles_buf.*;
        let wts: []f64 = weights_buf.*;
        const init_w: f64 = 1.0 / (n_particles as f64);

        let i: usize = 0;
        while (i < n_particles) {
            const row: usize = i * dim;
            let j: usize = 0;
            while (j < dim) {
                const s1: u64 = splitMixStep(rng);
                const s2: u64 = splitMixStep(s1);
                rng = s2;
                pts[row + j] = mean[j] + (std_dev[j] * normalFromPair(s1, s2));
                j += 1;
            }
            wts[i] = init_w;
            i += 1;
        }

        return ParticleFilter {
            n_particles: n_particles,
            dim: dim,
            particles: pts,
            weights: wts,
            rng_state: rng,
        };
    }

    /// Propagates every particle through `prop_fn` with fresh Gaussian process noise scaled by `process_std`.
    fn predict(
        *_,
        prop_fn: PropagateFn,
        u: []const f64,
        dt: f64,
        process_std: []const f64,
        noise_scratch: *[]f64,
    ): bool {
        if (_.n_particles == 0 || _.dim == 0 || !isFiniteF64(dt)) {
            return false;
        }
        if (process_std.len < _.dim || noise_scratch.*.len < _.dim) {
            return false;
        }
        if (!checkFinitePrefix(u, u.len)) {
            return false;
        }
        let d: usize = 0;
        while (d < _.dim) {
            if (!isFiniteF64(process_std[d]) || process_std[d] < 0.0) {
                return false;
            }
            d += 1;
        }

        let noise: []f64 = noise_scratch.*;
        let i: usize = 0;
        while (i < _.n_particles) {
            let j: usize = 0;
            while (j < _.dim) {
                const s1: u64 = splitMixStep(_.rng_state);
                const s2: u64 = splitMixStep(s1);
                _.rng_state = s2;
                noise[j] = process_std[j] * normalFromPair(s1, s2);
                j += 1;
            }
            const row: usize = i * _.dim;
            let st: []f64 = _.particles[row..(row + _.dim)];
            prop_fn(*st, u, dt, noise[0.._.dim]);
            if (!checkFinitePrefix(st, _.dim)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// Updates particle weights using observation log-likelihood `ll_fn` and log-sum-exp normalization.
    /// Returns the log marginal likelihood `log p(z_t | z_{1:t-1})`.
    fn update(
        *_,
        ll_fn: LogLikelihoodFn,
        z: []const f64,
        log_w_scratch: *[]f64,
    ): f64? {
        if (_.n_particles == 0 || _.dim == 0 || log_w_scratch.*.len < _.n_particles) {
            return null;
        }
        if (!checkFinitePrefix(z, z.len)) {
            return null;
        }

        let lw: []f64 = log_w_scratch.*;
        let i: usize = 0;
        while (i < _.n_particles) {
            const w_cur: f64 = _.weights[i];
            if (!isFiniteF64(w_cur) || w_cur < 0.0) {
                return null;
            }
            if (w_cur <= 0.0) {
                lw[i] = negInfF64();
            } else {
                const row: usize = i * _.dim;
                const ll: f64 = ll_fn(_.particles[row..(row + _.dim)], z);
                if (ll != ll || (!isFiniteF64(ll) && ll > 0.0)) {
                    return null;
                }
                lw[i] = if (isFiniteF64(ll)) (@log(w_cur) + ll) else negInfF64();
            }
            i += 1;
        }

        let w_slice: []f64 = _.weights[0.._.n_particles];
        return normalizeLogWeights(lw[0.._.n_particles], *w_slice);
    }

    /// Triggers particle resampling when `ESS < ess_threshold_ratio * n_particles` and resets uniform weights `1/N`.
    /// Returns `true` if resampling was executed, `false` otherwise.
    fn resampleIfNeeded(
        *_,
        ess_threshold_ratio: f64,
        kind: ResampleKind,
        indices_s: *[]usize,
        tmp_particles: *[]f64,
    ): bool {
        if (
            _.n_particles == 0 ||
            _.dim == 0 ||
            !isFiniteF64(ess_threshold_ratio) ||
            ess_threshold_ratio <= 0.0 ||
            indices_s.*.len < _.n_particles ||
            !checkMatrixBounds(tmp_particles.*.len, _.n_particles, _.dim)
        ) {
            return false;
        }

        const ess: f64 = effectiveSampleSize(_.weights[0.._.n_particles]);
        const thresh: f64 = ess_threshold_ratio * (_.n_particles as f64);
        if (ess >= thresh) {
            return false;
        }

        _.rng_state = splitMixStep(_.rng_state);
        const sub_seed: u64 = splitMixMix(_.rng_state);
        if (!resampleIndices(_.weights[0.._.n_particles], kind, sub_seed, indices_s)) {
            return false;
        }

        const indices: []const usize = indices_s.*;
        let tmp: []f64 = tmp_particles.*;
        let i: usize = 0;
        while (i < _.n_particles) {
            const src_row: usize = indices[i] * _.dim;
            const dst_row: usize = i * _.dim;
            let j: usize = 0;
            while (j < _.dim) {
                tmp[dst_row + j] = _.particles[src_row + j];
                j += 1;
            }
            i += 1;
        }

        const total_floats: usize = _.n_particles * _.dim;
        let k: usize = 0;
        while (k < total_floats) {
            _.particles[k] = tmp[k];
            k += 1;
        }

        const uniform_w: f64 = 1.0 / (_.n_particles as f64);
        let m: usize = 0;
        while (m < _.n_particles) {
            _.weights[m] = uniform_w;
            m += 1;
        }
        return true;
    }

    /// Computes the weighted posterior mean state into `mean_out[0..dim]`.
    fn estimateMean(mean_out: *[]f64): bool {
        if (_.n_particles == 0 || _.dim == 0 || mean_out.*.len < _.dim) {
            return false;
        }
        let dst: []f64 = mean_out.*;
        let j0: usize = 0;
        while (j0 < _.dim) {
            dst[j0] = 0.0;
            j0 += 1;
        }
        let i: usize = 0;
        while (i < _.n_particles) {
            const w: f64 = _.weights[i];
            const row: usize = i * _.dim;
            let j: usize = 0;
            while (j < _.dim) {
                dst[j] += w * _.particles[row + j];
                j += 1;
            }
            i += 1;
        }
        return true;
    }

    /// Computes the weighted posterior `dim x dim` covariance matrix into row-major `cov_out[0..dim*dim]`.
    fn estimateCovariance(mean: []const f64, cov_out: *[]f64): bool {
        if (
            _.n_particles == 0 ||
            _.dim == 0 ||
            mean.len < _.dim ||
            !checkMatrixBounds(cov_out.*.len, _.dim, _.dim) ||
            !checkFinitePrefix(mean, _.dim)
        ) {
            return false;
        }
        let cov: []f64 = cov_out.*;
        const total_cov: usize = _.dim * _.dim;
        let z: usize = 0;
        while (z < total_cov) {
            cov[z] = 0.0;
            z += 1;
        }

        let i: usize = 0;
        while (i < _.n_particles) {
            const w: f64 = _.weights[i];
            const row: usize = i * _.dim;
            let a: usize = 0;
            while (a < _.dim) {
                const da: f64 = _.particles[row + a] - mean[a];
                const w_da: f64 = w * da;
                let b: usize = a;
                while (b < _.dim) {
                    const db: f64 = _.particles[row + b] - mean[b];
                    cov[a * _.dim + b] += w_da * db;
                    b += 1;
                }
                a += 1;
            }
            i += 1;
        }

        let a2: usize = 0;
        while (a2 < _.dim) {
            let b2: usize = a2 + 1;
            while (b2 < _.dim) {
                cov[b2 * _.dim + a2] = cov[a2 * _.dim + b2];
                b2 += 1;
            }
            a2 += 1;
        }
        return true;
    }
}
