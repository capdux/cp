// Origin: Pure-CP Stdlib (Zero-Alloc Bessel J0/J1/Jn, Y0/Y1/Yn & Modified Bessel I0/I1/I0e/I1e/K0/K1/K0e/K1e) | Created: 2026-11-22

import std from "std";

const PI: f64 = 3.14159265358979323846;
const TWO_OVER_PI: f64 = 0.63661977236758134308;
const SQRT_2_OVER_PI: f64 = 0.79788456080286535588;
const SQRT_PI_OVER_2: f64 = 1.25331413731550025121;
const INV_SQRT_2PI: f64 = 0.39894228040143267794;
const LN2: f64 = 0.69314718055994530942;

// Euler-Mascheroni constant gamma_E in double-double precision (~32 digits)
const EULER_HI: f64 = 0.57721566490153286061;
const EULER_LO: f64 = -4.942915152430649e-17;

// Double-double accumulator for lossless series summation (106-bit significand)
struct Dd {
    hi: f64,
    lo: f64,
}

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn isNanF64(x: f64): bool {
    return x != x;
}

fn nanF64(): f64 {
    const z: f64 = 0.0;
    return z / z;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) (0.0 - x) else x;
}

fn twoSum(a: f64, b: f64): Dd {
    const s: f64 = a + b;
    const bb: f64 = s - a;
    const err: f64 = (a - (s - bb)) + (b - bb);
    return Dd { hi: s, lo: err };
}

fn ddAdd(a: Dd, b: Dd): Dd {
    const s: Dd = twoSum(a.hi, b.hi);
    const t: Dd = twoSum(a.lo, b.lo);
    const c: f64 = s.lo + t.hi;
    const v: Dd = twoSum(s.hi, c);
    return twoSum(v.hi, v.lo + t.lo);
}

fn ddSub(a: Dd, b: Dd): Dd {
    return ddAdd(a, Dd { hi: 0.0 - b.hi, lo: 0.0 - b.lo });
}

fn splitF64(a: f64): Dd {
    const c: f64 = 134217729.0 * a; // (2^27 + 1) * a
    const hi: f64 = c - (c - a);
    const lo: f64 = a - hi;
    return Dd { hi: hi, lo: lo };
}

fn twoProd(a: f64, b: f64): Dd {
    const p: f64 = a * b;
    const sa: Dd = splitF64(a);
    const sb: Dd = splitF64(b);
    const err: f64 = ((sa.hi * sb.hi - p) + sa.hi * sb.lo + sa.lo * sb.hi) + sa.lo * sb.lo;
    return Dd { hi: p, lo: err };
}

fn ddMul(a: Dd, b: Dd): Dd {
    const p: Dd = twoProd(a.hi, b.hi);
    const err: f64 = p.lo + (a.hi * b.lo + a.lo * b.hi);
    return twoSum(p.hi, err);
}

fn ddMulF64(a: Dd, b: f64): Dd {
    return ddMul(a, Dd { hi: b, lo: 0.0 });
}

fn ddDivF64(a: Dd, b: f64): Dd {
    const q1: f64 = a.hi / b;
    const p1: Dd = twoProd(q1, b);
    const r: Dd = ddSub(a, p1);
    const q2: f64 = r.hi / b;
    return twoSum(q1, q2);
}

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

/// Computes double-double ln(x) via range reduction + atanh series (~32 digits).
fn ddLn(x: f64): Dd {
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
    const one: Dd = Dd { hi: 1.0, lo: 0.0 };
    const dm: Dd = Dd { hi: m, lo: 0.0 };
    const num: Dd = ddSub(dm, one);
    const den: Dd = ddAdd(dm, one);
    // t = num / den
    const t_hi: f64 = num.hi / den.hi;
    const t_rem: Dd = ddSub(num, ddMulF64(den, t_hi));
    const t: Dd = twoSum(t_hi, t_rem.hi / den.hi);
    const t2: Dd = ddMul(t, t);

    let term: Dd = t;
    let sum: Dd = Dd { hi: 0.0, lo: 0.0 };
    let d: f64 = 1.0;
    let iter: usize = 0;
    while (iter < 24) {
        sum = ddAdd(sum, ddDivF64(term, d));
        term = ddMul(term, t2);
        d += 2.0;
        if (absF64(term.hi) < 1.0e-33) {
            break;
        }
        iter += 1;
    }
    const ln2_dd: Dd = Dd { hi: 0.69314718055994530942, lo: 2.3190468138462996e-17 };
    const part1: Dd = ddMulF64(sum, 2.0);
    const part2: Dd = ddMulF64(ln2_dd, k as f64);
    return ddAdd(part1, part2);
}

// Hankel asymptotic P_nu(x) and Q_nu(x) for large x (> 18.0)
struct HankelPQ {
    p: f64,
    q: f64,
}

fn hankelPQ(nu: f64, x: f64): HankelPQ {
    const mu: f64 = 4.0 * nu * nu;
    const inv8x: f64 = 1.0 / (8.0 * x);
    let p_sum: f64 = 1.0;
    let q_sum: f64 = 0.0;
    let term: f64 = 1.0;
    let prev_abs: f64 = 1.0e300;
    let p_sign: f64 = -1.0;
    let q_sign: f64 = 1.0;

    let m: usize = 1;
    while (m <= 40) {
        const fm: f64 = m as f64;
        const odd: f64 = 2.0 * fm - 1.0;
        term = term * (mu - odd * odd) * (inv8x / fm);
        const cur_abs: f64 = absF64(term);
        if (m > 4 && cur_abs > prev_abs) {
            break;
        }
        if ((m % 2) == 1) {
            q_sum += q_sign * term;
            q_sign = 0.0 - q_sign;
        } else {
            p_sum += p_sign * term;
            p_sign = 0.0 - p_sign;
        }
        if (cur_abs < 1.0e-17) {
            break;
        }
        prev_abs = cur_abs;
        m += 1;
    }
    return HankelPQ { p: p_sum, q: q_sum };
}

fn j0SeriesDd(x: f64): Dd {
    const half_x: Dd = Dd { hi: 0.5 * x, lo: 0.0 };
    const neg_z: Dd = ddMulF64(ddMul(half_x, half_x), -1.0);
    let term: Dd = Dd { hi: 1.0, lo: 0.0 };
    let sum: Dd = term;
    let k: usize = 1;
    while (k <= 80) {
        const fk: f64 = k as f64;
        term = ddDivF64(ddDivF64(ddMul(term, neg_z), fk), fk);
        sum = ddAdd(sum, term);
        if (absF64(term.hi) <= absF64(sum.hi) * 1.0e-32) {
            break;
        }
        k += 1;
    }
    return sum;
}

fn j1SeriesDd(x: f64): Dd {
    const half_x: Dd = Dd { hi: 0.5 * x, lo: 0.0 };
    const neg_z: Dd = ddMulF64(ddMul(half_x, half_x), -1.0);
    let term: Dd = half_x;
    let sum: Dd = term;
    let k: usize = 1;
    while (k <= 80) {
        const fk: f64 = k as f64;
        term = ddDivF64(ddDivF64(ddMul(term, neg_z), fk), fk + 1.0);
        sum = ddAdd(sum, term);
        if (absF64(term.hi) <= absF64(sum.hi) * 1.0e-32) {
            break;
        }
        k += 1;
    }
    return sum;
}

/// First-kind cylindrical Bessel function `J_0(x)` (even: `J_0(-x) == J_0(x)`).
pub fn j0(x: f64): f64 {
    if (isNanF64(x)) {
        return x;
    }
    if (!isFiniteF64(x)) {
        return 0.0;
    }
    const ax: f64 = absF64(x);
    if (ax == 0.0) {
        return 1.0;
    }
    if (ax <= 18.0) {
        const res: Dd = j0SeriesDd(ax);
        return res.hi + res.lo;
    }
    const pq: HankelPQ = hankelPQ(0.0, ax);
    const theta: f64 = ax - 0.25 * PI;
    return (SQRT_2_OVER_PI / std.math.sqrt(ax))
        * (pq.p * std.math.cos(theta) - pq.q * std.math.sin(theta));
}

/// First-kind cylindrical Bessel function `J_1(x)` (odd: `J_1(-x) == -J_1(x)`).
pub fn j1(x: f64): f64 {
    if (isNanF64(x)) {
        return x;
    }
    if (!isFiniteF64(x)) {
        return 0.0;
    }
    if (x == 0.0) {
        return 0.0;
    }
    const sign: f64 = if (x < 0.0) -1.0 else 1.0;
    const ax: f64 = absF64(x);
    if (ax <= 18.0) {
        const res: Dd = j1SeriesDd(ax);
        return sign * (res.hi + res.lo);
    }
    const pq: HankelPQ = hankelPQ(1.0, ax);
    const theta: f64 = ax - 0.75 * PI;
    return sign * (SQRT_2_OVER_PI / std.math.sqrt(ax))
        * (pq.p * std.math.cos(theta) - pq.q * std.math.sin(theta));
}

/// First-kind cylindrical Bessel function `J_n(x)` of arbitrary integer order `n`.
/// Uses downward Miller recurrence when `|x| <= |n|` to prevent numerical instability.
pub fn jn(n: i32, x: f64): f64 {
    if (isNanF64(x)) {
        return x;
    }
    if (!isFiniteF64(x)) {
        return 0.0;
    }
    if (n == -2147483648) {
        return 0.0;
    }
    let ord: i32 = n;
    let sign: f64 = 1.0;
    if (ord < 0) {
        ord = 0 - ord;
        if ((ord & 1) != 0) {
            sign = 0.0 - sign;
        }
    }
    if (ord == 0) {
        return j0(x);
    }
    if (ord == 1) {
        return sign * j1(x);
    }
    if (x == 0.0) {
        return 0.0;
    }
    let ax: f64 = x;
    if (ax < 0.0) {
        ax = 0.0 - ax;
        if ((ord & 1) != 0) {
            sign = 0.0 - sign;
        }
    }

    const ord_u: usize = ord as usize;
    const ord_f: f64 = ord as f64;
    if (ax > ord_f) {
        // Stable upward recurrence when x > n
        let b0: f64 = j0(ax);
        let b1: f64 = j1(ax);
        let k: usize = 1;
        while (k < ord_u) {
            const b2: f64 = (2.0 * (k as f64) / ax) * b1 - b0;
            b0 = b1;
            b1 = b2;
            k += 1;
        }
        return sign * b1;
    }

    // Downward Miller recurrence when 0 < x <= n
    const extra: usize = (std.math.sqrt(40.0 * ord_f) as usize) + 24;
    let m_start: usize = ord_u + extra;
    if ((m_start % 2) == 1) {
        m_start += 1;
    }

    let b_next: f64 = 0.0;
    let b_curr: f64 = 1.0;
    let ans: f64 = 0.0;
    let norm: f64 = 0.0;

    let k: usize = m_start;
    while (k > 0) {
        const b_prev: f64 = (2.0 * (k as f64) / ax) * b_curr - b_next;
        b_next = b_curr;
        b_curr = b_prev;

        const idx: usize = k - 1;
        if (idx == ord_u) {
            ans = b_curr;
        }
        if (idx == 0) {
            norm += b_curr;
        } else if ((idx % 2) == 0) {
            norm += 2.0 * b_curr;
        }

        if (absF64(b_curr) > 1.0e100) {
            const inv_scale: f64 = 1.0e-100;
            b_curr *= inv_scale;
            b_next *= inv_scale;
            ans *= inv_scale;
            norm *= inv_scale;
        }
        k -= 1;
    }

    if (norm == 0.0 || !isFiniteF64(norm)) {
        return 0.0;
    }
    return sign * (ans / norm);
}

/// Second-kind cylindrical Bessel (Neumann) function `Y_0(x)` for `x > 0`.
/// Returns `-inf` at `x == 0` and `NaN` for `x < 0` or `NaN`.
pub fn y0(x: f64): f64 {
    if (isNanF64(x) || x < 0.0) {
        return nanF64();
    }
    if (x == 0.0) {
        return 0.0 - std.math.inf(f64);
    }
    if (!isFiniteF64(x)) {
        return 0.0;
    }
    if (x <= 18.0) {
        const j0_dd: Dd = j0SeriesDd(x);
        const half_x: Dd = Dd { hi: 0.5 * x, lo: 0.0 };
        const neg_z: Dd = ddMulF64(ddMul(half_x, half_x), -1.0);
        const ln_half: Dd = ddLn(0.5 * x);
        const coeff: Dd = ddAdd(ln_half, Dd { hi: EULER_HI, lo: EULER_LO });

        let term: Dd = Dd { hi: 1.0, lo: 0.0 };
        let h_k: Dd = Dd { hi: 0.0, lo: 0.0 };
        let h_sum: Dd = Dd { hi: 0.0, lo: 0.0 };
        let k: usize = 1;
        while (k <= 80) {
            const fk: f64 = k as f64;
            h_k = ddAdd(h_k, ddDivF64(Dd { hi: 1.0, lo: 0.0 }, fk));
            term = ddDivF64(ddDivF64(ddMul(term, neg_z), fk), fk);
            const contrib: Dd = ddMul(term, h_k);
            h_sum = ddAdd(h_sum, contrib);
            if (absF64(contrib.hi) <= 1.0e-32) {
                break;
            }
            k += 1;
        }
        const inner: Dd = ddSub(ddMul(coeff, j0_dd), h_sum);
        return TWO_OVER_PI * (inner.hi + inner.lo);
    }
    const pq: HankelPQ = hankelPQ(0.0, x);
    const theta: f64 = x - 0.25 * PI;
    return (SQRT_2_OVER_PI / std.math.sqrt(x))
        * (pq.p * std.math.sin(theta) + pq.q * std.math.cos(theta));
}

/// Second-kind cylindrical Bessel (Neumann) function `Y_1(x)` for `x > 0`.
/// Returns `-inf` at `x == 0` and `NaN` for `x < 0` or `NaN`.
pub fn y1(x: f64): f64 {
    if (isNanF64(x) || x < 0.0) {
        return nanF64();
    }
    if (x == 0.0) {
        return 0.0 - std.math.inf(f64);
    }
    if (!isFiniteF64(x)) {
        return 0.0;
    }
    if (x <= 18.0) {
        const j1_dd: Dd = j1SeriesDd(x);
        const half_x: Dd = Dd { hi: 0.5 * x, lo: 0.0 };
        const neg_z: Dd = ddMulF64(ddMul(half_x, half_x), -1.0);
        const ln_half: Dd = ddLn(0.5 * x);
        const coeff: Dd = ddAdd(ln_half, Dd { hi: EULER_HI, lo: EULER_LO });

        let term: Dd = Dd { hi: 1.0, lo: 0.0 }; // (-z)^k / (k! (k+1)!)
        let h_k: Dd = Dd { hi: 0.0, lo: 0.0 };
        let h_k1: Dd = Dd { hi: 1.0, lo: 0.0 };
        let s_sum: Dd = h_k1; // k = 0 term: 1 * (H_0 + H_1) = 1
        let k: usize = 1;
        while (k <= 80) {
            const fk: f64 = k as f64;
            h_k = h_k1;
            h_k1 = ddAdd(h_k1, ddDivF64(Dd { hi: 1.0, lo: 0.0 }, fk + 1.0));
            term = ddDivF64(ddDivF64(ddMul(term, neg_z), fk), fk + 1.0);
            const contrib: Dd = ddMul(term, ddAdd(h_k, h_k1));
            s_sum = ddAdd(s_sum, contrib);
            if (absF64(contrib.hi) <= 1.0e-32) {
                break;
            }
            k += 1;
        }
        const inv_x: Dd = ddDivF64(Dd { hi: 1.0, lo: 0.0 }, x);
        const term1: Dd = ddSub(ddMul(coeff, j1_dd), inv_x);
        const inner: Dd = ddSub(term1, ddMulF64(s_sum, 0.25 * x));
        return TWO_OVER_PI * (inner.hi + inner.lo);
    }
    const pq: HankelPQ = hankelPQ(1.0, x);
    const theta: f64 = x - 0.75 * PI;
    return (SQRT_2_OVER_PI / std.math.sqrt(x))
        * (pq.p * std.math.sin(theta) + pq.q * std.math.cos(theta));
}

/// Second-kind cylindrical Bessel (Neumann) function `Y_n(x)` for integer `n` and `x > 0`.
pub fn yn(n: i32, x: f64): f64 {
    if (isNanF64(x) || x < 0.0) {
        return nanF64();
    }
    if (n == -2147483648) {
        return nanF64();
    }
    let ord: i32 = n;
    let sign: f64 = 1.0;
    if (ord < 0) {
        ord = 0 - ord;
        if ((ord & 1) != 0) {
            sign = 0.0 - sign;
        }
    }
    if (x == 0.0) {
        return sign * (0.0 - std.math.inf(f64));
    }
    if (!isFiniteF64(x)) {
        return 0.0;
    }
    if (ord == 0) {
        return y0(x);
    }
    if (ord == 1) {
        return sign * y1(x);
    }

    const ord_u: usize = ord as usize;
    let b0: f64 = y0(x);
    let b1: f64 = y1(x);
    let k: usize = 1;
    while (k < ord_u) {
        const b2: f64 = (2.0 * (k as f64) / x) * b1 - b0;
        if (!isFiniteF64(b2)) {
            return sign * b2;
        }
        b0 = b1;
        b1 = b2;
        k += 1;
    }
    return sign * b1;
}

fn i0SeriesDd(x: f64): Dd {
    const half_x: Dd = Dd { hi: 0.5 * x, lo: 0.0 };
    const z: Dd = ddMul(half_x, half_x);
    let term: Dd = Dd { hi: 1.0, lo: 0.0 };
    let sum: Dd = term;
    let k: usize = 1;
    while (k <= 80) {
        const fk: f64 = k as f64;
        term = ddDivF64(ddDivF64(ddMul(term, z), fk), fk);
        sum = ddAdd(sum, term);
        if (absF64(term.hi) <= absF64(sum.hi) * 1.0e-32) {
            break;
        }
        k += 1;
    }
    return sum;
}

fn i1SeriesDd(x: f64): Dd {
    const half_x: Dd = Dd { hi: 0.5 * x, lo: 0.0 };
    const z: Dd = ddMul(half_x, half_x);
    let term: Dd = half_x;
    let sum: Dd = term;
    let k: usize = 1;
    while (k <= 80) {
        const fk: f64 = k as f64;
        term = ddDivF64(ddDivF64(ddMul(term, z), fk), fk + 1.0);
        sum = ddAdd(sum, term);
        if (absF64(term.hi) <= absF64(sum.hi) * 1.0e-32) {
            break;
        }
        k += 1;
    }
    return sum;
}

fn iScaledAsymp(nu: f64, x: f64): f64 {
    const mu: f64 = 4.0 * nu * nu;
    const inv8x: f64 = 1.0 / (8.0 * x);
    let sum: f64 = 1.0;
    let term: f64 = 1.0;
    let prev_abs: f64 = 1.0e300;
    let m: usize = 1;
    while (m <= 40) {
        const fm: f64 = m as f64;
        const odd: f64 = 2.0 * fm - 1.0;
        term = term * (odd * odd - mu) * (inv8x / fm);
        const cur_abs: f64 = absF64(term);
        if (m > 4 && cur_abs > prev_abs) {
            break;
        }
        sum += term;
        if (cur_abs < 1.0e-17) {
            break;
        }
        prev_abs = cur_abs;
        m += 1;
    }
    return (INV_SQRT_2PI / std.math.sqrt(x)) * sum;
}

fn kScaledAsymp(nu: f64, x: f64): f64 {
    const mu: f64 = 4.0 * nu * nu;
    const inv8x: f64 = 1.0 / (8.0 * x);
    let sum: f64 = 1.0;
    let term: f64 = 1.0;
    let prev_abs: f64 = 1.0e300;
    let m: usize = 1;
    while (m <= 40) {
        const fm: f64 = m as f64;
        const odd: f64 = 2.0 * fm - 1.0;
        term = term * (mu - odd * odd) * (inv8x / fm);
        const cur_abs: f64 = absF64(term);
        if (m > 4 && cur_abs > prev_abs) {
            break;
        }
        sum += term;
        if (cur_abs < 1.0e-17) {
            break;
        }
        prev_abs = cur_abs;
        m += 1;
    }
    return (SQRT_PI_OVER_2 / std.math.sqrt(x)) * sum;
}

/// Exponentially scaled modified Bessel function `e^{-|x|} I_0(x)` (overflow-free for large `|x|`).
pub fn i0e(x: f64): f64 {
    if (isNanF64(x)) {
        return x;
    }
    if (!isFiniteF64(x)) {
        return 0.0;
    }
    const ax: f64 = absF64(x);
    if (ax == 0.0) {
        return 1.0;
    }
    if (ax <= 24.0) {
        const s: Dd = i0SeriesDd(ax);
        return (s.hi + s.lo) * std.math.exp(0.0 - ax);
    }
    return iScaledAsymp(0.0, ax);
}

/// Exponentially scaled modified Bessel function `e^{-|x|} I_1(x)` (odd, overflow-free for large `|x|`).
pub fn i1e(x: f64): f64 {
    if (isNanF64(x)) {
        return x;
    }
    if (!isFiniteF64(x)) {
        return 0.0;
    }
    if (x == 0.0) {
        return 0.0;
    }
    const sign: f64 = if (x < 0.0) -1.0 else 1.0;
    const ax: f64 = absF64(x);
    if (ax <= 24.0) {
        const s: Dd = i1SeriesDd(ax);
        return sign * (s.hi + s.lo) * std.math.exp(0.0 - ax);
    }
    return sign * iScaledAsymp(1.0, ax);
}

/// First-kind modified Bessel function `I_0(x)` (even: `I_0(-x) == I_0(x)`).
/// Named `besselI0` to avoid shadowing Zig's `i0` primitive integer type.
pub fn besselI0(x: f64): f64 {
    if (isNanF64(x)) {
        return x;
    }
    if (!isFiniteF64(x)) {
        return std.math.inf(f64);
    }
    const ax: f64 = absF64(x);
    if (ax == 0.0) {
        return 1.0;
    }
    if (ax <= 24.0) {
        const s: Dd = i0SeriesDd(ax);
        return s.hi + s.lo;
    }
    return std.math.exp(ax) * iScaledAsymp(0.0, ax);
}

/// First-kind modified Bessel function `I_1(x)` (odd: `I_1(-x) == -I_1(x)`).
/// Named `besselI1` to avoid shadowing Zig's `i1` primitive integer type.
pub fn besselI1(x: f64): f64 {
    if (isNanF64(x)) {
        return x;
    }
    if (!isFiniteF64(x)) {
        return if (x > 0.0) std.math.inf(f64) else (0.0 - std.math.inf(f64));
    }
    if (x == 0.0) {
        return 0.0;
    }
    const sign: f64 = if (x < 0.0) -1.0 else 1.0;
    const ax: f64 = absF64(x);
    if (ax <= 24.0) {
        const s: Dd = i1SeriesDd(ax);
        return sign * (s.hi + s.lo);
    }
    return sign * std.math.exp(ax) * iScaledAsymp(1.0, ax);
}

fn k0SeriesDd(x: f64): f64 {
    const i0_dd: Dd = i0SeriesDd(x);
    const half_x: Dd = Dd { hi: 0.5 * x, lo: 0.0 };
    const z: Dd = ddMul(half_x, half_x);
    const ln_half: Dd = ddLn(0.5 * x);
    const coeff: Dd = ddAdd(ln_half, Dd { hi: EULER_HI, lo: EULER_LO });

    let term: Dd = Dd { hi: 1.0, lo: 0.0 };
    let h_k: Dd = Dd { hi: 0.0, lo: 0.0 };
    let h_sum: Dd = Dd { hi: 0.0, lo: 0.0 };
    let k: usize = 1;
    while (k <= 80) {
        const fk: f64 = k as f64;
        h_k = ddAdd(h_k, ddDivF64(Dd { hi: 1.0, lo: 0.0 }, fk));
        term = ddDivF64(ddDivF64(ddMul(term, z), fk), fk);
        const contrib: Dd = ddMul(term, h_k);
        h_sum = ddAdd(h_sum, contrib);
        if (absF64(contrib.hi) <= absF64(h_sum.hi) * 1.0e-32) {
            break;
        }
        k += 1;
    }
    const res: Dd = ddSub(h_sum, ddMul(coeff, i0_dd));
    return res.hi + res.lo;
}

fn k1SeriesDd(x: f64): f64 {
    const i1_dd: Dd = i1SeriesDd(x);
    const half_x: Dd = Dd { hi: 0.5 * x, lo: 0.0 };
    const z: Dd = ddMul(half_x, half_x);
    const ln_half: Dd = ddLn(0.5 * x);
    const coeff: Dd = ddAdd(ln_half, Dd { hi: EULER_HI, lo: EULER_LO });

    let term: Dd = Dd { hi: 1.0, lo: 0.0 };
    let h_k: Dd = Dd { hi: 0.0, lo: 0.0 };
    let h_k1: Dd = Dd { hi: 1.0, lo: 0.0 };
    let s_sum: Dd = h_k1;
    let k: usize = 1;
    while (k <= 80) {
        const fk: f64 = k as f64;
        h_k = h_k1;
        h_k1 = ddAdd(h_k1, ddDivF64(Dd { hi: 1.0, lo: 0.0 }, fk + 1.0));
        term = ddDivF64(ddDivF64(ddMul(term, z), fk), fk + 1.0);
        const contrib: Dd = ddMul(term, ddAdd(h_k, h_k1));
        s_sum = ddAdd(s_sum, contrib);
        if (absF64(contrib.hi) <= absF64(s_sum.hi) * 1.0e-32) {
            break;
        }
        k += 1;
    }
    const inv_x: Dd = ddDivF64(Dd { hi: 1.0, lo: 0.0 }, x);
    const t1: Dd = ddAdd(inv_x, ddMul(coeff, i1_dd));
    const res: Dd = ddSub(t1, ddMulF64(s_sum, 0.25 * x));
    return res.hi + res.lo;
}

/// Exponentially scaled second-kind modified Bessel function `e^x K_0(x)` (`x > 0`, underflow-free).
pub fn k0e(x: f64): f64 {
    if (isNanF64(x) || x < 0.0) {
        return nanF64();
    }
    if (x == 0.0) {
        return std.math.inf(f64);
    }
    if (!isFiniteF64(x)) {
        return 0.0;
    }
    if (x <= 16.0) {
        return std.math.exp(x) * k0SeriesDd(x);
    }
    return kScaledAsymp(0.0, x);
}

/// Exponentially scaled second-kind modified Bessel function `e^x K_1(x)` (`x > 0`, underflow-free).
pub fn k1e(x: f64): f64 {
    if (isNanF64(x) || x < 0.0) {
        return nanF64();
    }
    if (x == 0.0) {
        return std.math.inf(f64);
    }
    if (!isFiniteF64(x)) {
        return 0.0;
    }
    if (x <= 16.0) {
        return std.math.exp(x) * k1SeriesDd(x);
    }
    return kScaledAsymp(1.0, x);
}

/// Second-kind modified Bessel (Macdonald) function `K_0(x)` for `x > 0`.
pub fn k0(x: f64): f64 {
    if (isNanF64(x) || x < 0.0) {
        return nanF64();
    }
    if (x == 0.0) {
        return std.math.inf(f64);
    }
    if (!isFiniteF64(x)) {
        return 0.0;
    }
    if (x <= 16.0) {
        return k0SeriesDd(x);
    }
    return std.math.exp(0.0 - x) * kScaledAsymp(0.0, x);
}

/// Second-kind modified Bessel (Macdonald) function `K_1(x)` for `x > 0`.
pub fn k1(x: f64): f64 {
    if (isNanF64(x) || x < 0.0) {
        return nanF64();
    }
    if (x == 0.0) {
        return std.math.inf(f64);
    }
    if (!isFiniteF64(x)) {
        return 0.0;
    }
    if (x <= 16.0) {
        return k1SeriesDd(x);
    }
    return std.math.exp(0.0 - x) * kScaledAsymp(1.0, x);
}
