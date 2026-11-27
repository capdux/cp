// Origin: Pure-CP Stdlib (Ken Perlin 2002 Zero-Alloc Improved Perlin Noise 1D/2D/3D, fBm, Ridged & Turbulence) | Created: 2026-11-22

import std from "std";

const CLASSIC_PERM_256: [256]u8 = [
    151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225,
    140, 36,  103, 30,  69,  142, 8,   99,  37,  240, 21,  10,  23,  190, 6,   148,
    247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203, 117, 35,  11,  32,
    57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136, 171, 168, 68,  175,
    74,  165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122,
    60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245, 40,  244, 102, 143, 54,
    65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,  18,  169,
    200, 196, 135, 130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,
    52,  217, 226, 250, 124, 123, 5,   202, 38,  147, 118, 126, 255, 82,  85,  212,
    207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,  223, 183, 170, 213,
    119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,
    129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104,
    218, 246, 97,  228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241,
    81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181, 199, 106, 157,
    184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,
    222, 114, 67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180,
];

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

/// Ken Perlin 2002 quintic C^2 smoothstep fade curve: `6t^5 - 15t^4 + 10t^3`.
pub fn fade(t: f64): f64 {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

/// Linear interpolation between `a` and `b` at factor `t`.
pub fn lerp(a: f64, b: f64, t: f64): f64 {
    return a + t * (b - a);
}

fn grad1(hash: u8, x: f64): f64 {
    return if ((hash & 1) == 0) x else (0.0 - x);
}

fn grad3(hash: u8, x: f64, y: f64, z: f64): f64 {
    const h = hash & 15;
    const u: f64 = if (h < 8) x else y;
    const v: f64 = if (h < 4) y else if (h == 12 || h == 14) x else z;
    const a: f64 = if ((h & 1) == 0) u else (0.0 - u);
    const b: f64 = if ((h & 2) == 0) v else (0.0 - v);
    return a + b;
}

fn xorshift64Next(state: u64): u64 {
    let x = state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return x;
}

/// Zero-alloc Ken Perlin 2002 Improved Perlin Noise generator with a 512-byte stack permutation table.
pub struct PerlinNoise {
    perm: [512]u8,

    /// Initializes with Ken Perlin's classic 2002 reference permutation table.
    static fn classic(): PerlinNoise {
        let p: [512]u8 = undefined;
        @memcpy(p[0..256], CLASSIC_PERM_256[0..256]);
        @memcpy(p[256..512], CLASSIC_PERM_256[0..256]);
        return PerlinNoise { perm: p };
    }

    /// Deterministically initializes a permutation table from a 64-bit `seed`
    /// using overflow-free xorshift64 + Fisher-Yates shuffle.
    static fn fromSeed(seed: u64): PerlinNoise {
        let p: [512]u8 = undefined;
        let i: usize = 0;
        while (i < 256) {
            p[i] = i as u8;
            i += 1;
        }

        let rng: u64 = seed ^ 0x9E3779B97F4A7C15;
        if (rng == 0) {
            rng = 0xBF58476D1CE4E5B9;
        }
        // Warm up state
        rng = xorshift64Next(rng);
        rng = xorshift64Next(rng);
        rng = xorshift64Next(rng);

        let idx: usize = 255;
        while (idx > 0) {
            rng = xorshift64Next(rng);
            const j: usize = (rng % ((idx + 1) as u64)) as usize;
            const tmp = p[idx];
            p[idx] = p[j];
            p[j] = tmp;
            idx -= 1;
        }

        @memcpy(p[256..512], p[0..256]);
        return PerlinNoise { perm: p };
    }

    /// 1D Improved Perlin Noise in `[-1, 1]`, strictly `0.0` at integer coordinates.
    fn noise1d(x: f64): f64 {
        const fx = std.math.floor(x);
        const xi: usize = ((fx as i64) & 255) as usize;
        const xf = x - fx;
        const u = fade(xf);

        const a = _.perm[xi];
        const b = _.perm[xi + 1];
        return 2.0 * lerp(grad1(a, xf), grad1(b, xf - 1.0), u);
    }

    /// 2D Improved Perlin Noise in `[-1, 1]`, strictly `0.0` at integer grid points.
    fn noise2d(x: f64, y: f64): f64 {
        const fx = std.math.floor(x);
        const fy = std.math.floor(y);
        const xi: usize = ((fx as i64) & 255) as usize;
        const yi: usize = ((fy as i64) & 255) as usize;
        const xf = x - fx;
        const yf = y - fy;

        const u = fade(xf);
        const v = fade(yf);

        const aa = _.perm[(_.perm[xi] as usize) + yi];
        const ab = _.perm[(_.perm[xi] as usize) + yi + 1];
        const ba = _.perm[(_.perm[xi + 1] as usize) + yi];
        const bb = _.perm[(_.perm[xi + 1] as usize) + yi + 1];

        const x1 = lerp(grad3(aa, xf, yf, 0.0), grad3(ba, xf - 1.0, yf, 0.0), u);
        const x2 = lerp(grad3(ab, xf, yf - 1.0, 0.0), grad3(bb, xf - 1.0, yf - 1.0, 0.0), u);
        return lerp(x1, x2, v);
    }

    /// 3D Improved Perlin Noise in `[-1, 1]`, strictly `0.0` at integer lattice points.
    fn noise3d(x: f64, y: f64, z: f64): f64 {
        const fx = std.math.floor(x);
        const fy = std.math.floor(y);
        const fz = std.math.floor(z);
        const xi: usize = ((fx as i64) & 255) as usize;
        const yi: usize = ((fy as i64) & 255) as usize;
        const zi: usize = ((fz as i64) & 255) as usize;

        const xf = x - fx;
        const yf = y - fy;
        const zf = z - fz;

        const u = fade(xf);
        const v = fade(yf);
        const w = fade(zf);

        const a: usize = (_.perm[xi] as usize) + yi;
        const aa: usize = (_.perm[a] as usize) + zi;
        const ab: usize = (_.perm[a + 1] as usize) + zi;
        const b: usize = (_.perm[xi + 1] as usize) + yi;
        const ba: usize = (_.perm[b] as usize) + zi;
        const bb: usize = (_.perm[b + 1] as usize) + zi;

        const x1 = lerp(grad3(_.perm[aa], xf, yf, zf), grad3(_.perm[ba], xf - 1.0, yf, zf), u);
        const x2 = lerp(grad3(_.perm[ab], xf, yf - 1.0, zf), grad3(_.perm[bb], xf - 1.0, yf - 1.0, zf), u);
        const y1 = lerp(x1, x2, v);

        const x3 = lerp(grad3(_.perm[aa + 1], xf, yf, zf - 1.0), grad3(_.perm[ba + 1], xf - 1.0, yf, zf - 1.0), u);
        const x4 = lerp(grad3(_.perm[ab + 1], xf, yf - 1.0, zf - 1.0), grad3(_.perm[bb + 1], xf - 1.0, yf - 1.0, zf - 1.0), u);
        const y2 = lerp(x3, x4, v);

        return lerp(y1, y2, w);
    }

    /// 2D Fractal Brownian Motion normalized to `[-1, 1]`. Returns `0.0` if `octaves == 0`.
    fn fbm2d(x: f64, y: f64, octaves: u8, lacunarity: f64, gain: f64): f64 {
        if (octaves == 0) {
            return 0.0;
        }
        let sum: f64 = 0.0;
        let norm: f64 = 0.0;
        let amp: f64 = 1.0;
        let freq: f64 = 1.0;
        let i: u8 = 0;
        while (i < octaves) {
            sum += amp * _.noise2d(x * freq, y * freq);
            norm += amp;
            freq *= lacunarity;
            amp *= gain;
            i += 1;
        }
        return if (norm > 0.0) sum / norm else 0.0;
    }

    /// 3D Fractal Brownian Motion normalized to `[-1, 1]`. Returns `0.0` if `octaves == 0`.
    fn fbm3d(x: f64, y: f64, z: f64, octaves: u8, lacunarity: f64, gain: f64): f64 {
        if (octaves == 0) {
            return 0.0;
        }
        let sum: f64 = 0.0;
        let norm: f64 = 0.0;
        let amp: f64 = 1.0;
        let freq: f64 = 1.0;
        let i: u8 = 0;
        while (i < octaves) {
            sum += amp * _.noise3d(x * freq, y * freq, z * freq);
            norm += amp;
            freq *= lacunarity;
            amp *= gain;
            i += 1;
        }
        return if (norm > 0.0) sum / norm else 0.0;
    }

    /// 2D Ridged multifractal noise (`1 - |noise|`), normalized to `[0, 1]`. Returns `0.0` if `octaves == 0`.
    fn ridged2d(x: f64, y: f64, octaves: u8, lacunarity: f64, gain: f64): f64 {
        if (octaves == 0) {
            return 0.0;
        }
        let sum: f64 = 0.0;
        let norm: f64 = 0.0;
        let amp: f64 = 1.0;
        let freq: f64 = 1.0;
        let i: u8 = 0;
        while (i < octaves) {
            const ridge = 1.0 - absF64(_.noise2d(x * freq, y * freq));
            sum += amp * ridge;
            norm += amp;
            freq *= lacunarity;
            amp *= gain;
            i += 1;
        }
        return if (norm > 0.0) sum / norm else 0.0;
    }

    /// 2D Turbulence (`|noise|` octave sum), normalized to `[0, 1]`. Returns `0.0` if `octaves == 0`.
    fn turbulence2d(x: f64, y: f64, octaves: u8, lacunarity: f64, gain: f64): f64 {
        if (octaves == 0) {
            return 0.0;
        }
        let sum: f64 = 0.0;
        let norm: f64 = 0.0;
        let amp: f64 = 1.0;
        let freq: f64 = 1.0;
        let i: u8 = 0;
        while (i < octaves) {
            sum += amp * absF64(_.noise2d(x * freq, y * freq));
            norm += amp;
            freq *= lacunarity;
            amp *= gain;
            i += 1;
        }
        return if (norm > 0.0) sum / norm else 0.0;
    }
}
