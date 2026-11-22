// Source: cp/lib/noise/noise.cp (Deterministic Coherent Noise Engine)
// Pure CP, zero-C-dependency Perlin 2D/3D, Worley/Cellular 2D, fBm, Ridged Multifractal, and 2D heightmap generator.

const std = @import("std");

/// Error set for coherent noise and fractal generation operations.
pub err NoiseError {
    InvalidOctaves,
    InvalidDimensions,
    InvalidFrequency,
    OutOfMemory,
}

/// Configuration for multi-octave fractal noise synthesis (`fbm2`, `ridged2`, `generateGrid2D`).
pub struct FractalConfig {
    octaves: u32 = 4;
    frequency: f64 = 1.0;
    lacunarity: f64 = 2.0;
    gain: f64 = 0.5;

    /// Constructs a `FractalConfig` with explicit parameters.
    static fn init(octaves: u32, frequency: f64, lacunarity: f64, gain: f64): FractalConfig {
        return FractalConfig {
            octaves: octaves,
            frequency: frequency,
            lacunarity: lacunarity,
            gain: gain,
        };
    }

    /// Returns a standard 4-octave fractal configuration.
    static fn default(): FractalConfig {
        return FractalConfig {
            octaves: 4,
            frequency: 1.0,
            lacunarity: 2.0,
            gain: 0.5,
        };
    }
}

fn splitmix64(state: *u64): u64 {
    state.* = state.* +% 0x9E3779B97F4A7C15;
    let z = state.*;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn fade(t: f64): f64 {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

fn lerp(a: f64, b: f64, t: f64): f64 {
    return a + t * (b - a);
}

fn clampF64(v: f64, lo: f64, hi: f64): f64 {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

fn grad2(hash: u8, x: f64, y: f64): f64 {
    switch (hash & 7) {
        0 => return x + y,
        1 => return -x + y,
        2 => return x - y,
        3 => return -x - y,
        4 => return x,
        5 => return -x,
        6 => return y,
        else => return -y,
    }
}

fn grad3(hash: u8, x: f64, y: f64, z: f64): f64 {
    switch (hash & 15) {
        0 => return x + y,
        1 => return -x + y,
        2 => return x - y,
        3 => return -x - y,
        4 => return x + z,
        5 => return -x + z,
        6 => return x - z,
        7 => return -x - z,
        8 => return y + z,
        9 => return -y + z,
        10 => return y - z,
        11 => return -y - z,
        12 => return y + x,
        13 => return -y + z,
        14 => return y - x,
        else => return -y - z,
    }
}

/// Deterministic, zero-heap-allocation coherent noise generator backed by a 512-entry SplitMix64 permutation table.
pub struct NoiseGen {
    perm: [512]u8;

    /// Initializes a deterministic `NoiseGen` from a 64-bit seed using SplitMix64 Fisher-Yates shuffling.
    static fn init(seed: u64): NoiseGen {
        let p: [256]u8 = undefined;
        for (i of 0..256) {
            p[i] = i as u8;
        }

        let rng_state = seed;
        let idx: usize = 255;
        while (idx > 0) : (idx -= 1) {
            const j = usize(splitmix64(&rng_state) % (u64(idx) + 1));
            const tmp = p[idx];
            p[idx] = p[j];
            p[j] = tmp;
        }

        let perm: [512]u8 = undefined;
        for (i of 0..256) {
            perm[i] = p[i];
            perm[i + 256] = p[i];
        }

        return NoiseGen { perm: perm };
    }

    /// Evaluates 2D Perlin gradient noise with quintic C2-continuous interpolation. Output is in `[-1.0, 1.0]`.
    fn perlin2(x: f64, y: f64): f64 {
        const xf_floor = @floor(x);
        const yf_floor = @floor(y);
        const xi = xf_floor as i32;
        const yi = yf_floor as i32;

        const xf = x - xf_floor;
        const yf = y - yf_floor;

        const u = fade(xf);
        const v = fade(yf);

        const X = usize(xi & 255);
        const Y = usize(yi & 255);

        const aa = _.perm[usize(_.perm[X]) + Y];
        const ab = _.perm[usize(_.perm[X]) + Y + 1];
        const ba = _.perm[usize(_.perm[X + 1]) + Y];
        const bb = _.perm[usize(_.perm[X + 1]) + Y + 1];

        const x1 = lerp(grad2(aa, xf, yf), grad2(ba, xf - 1.0, yf), u);
        const x2 = lerp(grad2(ab, xf, yf - 1.0), grad2(bb, xf - 1.0, yf - 1.0), u);

        return clampF64(lerp(x1, x2, v), -1.0, 1.0);
    }

    /// Evaluates 3D Perlin gradient noise with quintic C2-continuous interpolation. Output is in `[-1.0, 1.0]`.
    fn perlin3(x: f64, y: f64, z: f64): f64 {
        const xf_floor = @floor(x);
        const yf_floor = @floor(y);
        const zf_floor = @floor(z);
        const xi = xf_floor as i32;
        const yi = yf_floor as i32;
        const zi = zf_floor as i32;

        const xf = x - xf_floor;
        const yf = y - yf_floor;
        const zf = z - zf_floor;

        const u = fade(xf);
        const v = fade(yf);
        const w = fade(zf);

        const X = usize(xi & 255);
        const Y = usize(yi & 255);
        const Z = usize(zi & 255);

        const A = usize(_.perm[X]) + Y;
        const AA = usize(_.perm[A]) + Z;
        const AB = usize(_.perm[A + 1]) + Z;
        const B = usize(_.perm[X + 1]) + Y;
        const BA = usize(_.perm[B]) + Z;
        const BB = usize(_.perm[B + 1]) + Z;

        const x1 = lerp(
            grad3(_.perm[AA], xf, yf, zf),
            grad3(_.perm[BA], xf - 1.0, yf, zf),
            u
        );
        const x2 = lerp(
            grad3(_.perm[AB], xf, yf - 1.0, zf),
            grad3(_.perm[BB], xf - 1.0, yf - 1.0, zf),
            u
        );
        const y1 = lerp(x1, x2, v);

        const x3 = lerp(
            grad3(_.perm[AA + 1], xf, yf, zf - 1.0),
            grad3(_.perm[BA + 1], xf - 1.0, yf, zf - 1.0),
            u
        );
        const x4 = lerp(
            grad3(_.perm[AB + 1], xf, yf - 1.0, zf - 1.0),
            grad3(_.perm[BB + 1], xf - 1.0, yf - 1.0, zf - 1.0),
            u
        );
        const y2 = lerp(x3, x4, v);

        return clampF64(lerp(y1, y2, w), -1.0, 1.0);
    }

    /// Evaluates 2D Worley / Cellular Voronoi Euclidean distance to the nearest feature point (`[0.0, ~1.414]`).
    fn worley2(x: f64, y: f64): f64 {
        const xf_floor = @floor(x);
        const yf_floor = @floor(y);
        const xi = xf_floor as i32;
        const yi = yf_floor as i32;

        let min_dist_sq: f64 = 100.0;

        let dy: i32 = -1;
        while (dy <= 1) : (dy += 1) {
            let dx: i32 = -1;
            while (dx <= 1) : (dx += 1) {
                const cx = xi + dx;
                const cy = yi + dy;

                const cx_u = usize(cx & 255);
                const cy_u = usize(cy & 255);
                const hx = _.perm[usize(_.perm[cx_u]) + cy_u];
                const hy = _.perm[usize(_.perm[(cx_u + 131) & 255]) + ((cy_u + 73) & 255)];

                const px = f64(cx) + (f64(hx) + 0.5) / 256.0;
                const py = f64(cy) + (f64(hy) + 0.5) / 256.0;

                const diff_x = px - x;
                const diff_y = py - y;
                const d_sq = diff_x * diff_x + diff_y * diff_y;
                if (d_sq < min_dist_sq) {
                    min_dist_sq = d_sq;
                }
            }
        }

        return @sqrt(min_dist_sq);
    }

    /// Evaluates normalized 2D Fractional Brownian Motion (fBm). Output is strictly in `[-1.0, 1.0]`.
    fn fbm2(x: f64, y: f64, cfg: FractalConfig): f64 ! NoiseError {
        if (cfg.octaves == 0) return NoiseError.InvalidOctaves;
        if (cfg.frequency <= 0.0) return NoiseError.InvalidFrequency;

        let total: f64 = 0.0;
        let max_amp: f64 = 0.0;
        let freq: f64 = cfg.frequency;
        let amp: f64 = 1.0;

        let i: u32 = 0;
        while (i < cfg.octaves) : (i += 1) {
            total += _.perlin2(x * freq, y * freq) * amp;
            max_amp += amp;
            freq *= cfg.lacunarity;
            amp *= cfg.gain;
        }

        if (max_amp == 0.0) return 0.0;
        return clampF64(total / max_amp, -1.0, 1.0);
    }

    /// Evaluates normalized 2D Ridged Multifractal noise (`1.0 - |perlin2|`). Output is in `[0.0, 1.0]`.
    fn ridged2(x: f64, y: f64, cfg: FractalConfig): f64 ! NoiseError {
        if (cfg.octaves == 0) return NoiseError.InvalidOctaves;
        if (cfg.frequency <= 0.0) return NoiseError.InvalidFrequency;

        let total: f64 = 0.0;
        let max_amp: f64 = 0.0;
        let freq: f64 = cfg.frequency;
        let amp: f64 = 1.0;

        let i: u32 = 0;
        while (i < cfg.octaves) : (i += 1) {
            const n = _.perlin2(x * freq, y * freq);
            const signal = 1.0 - @abs(n);
            total += signal * amp;
            max_amp += amp;
            freq *= cfg.lacunarity;
            amp *= cfg.gain;
        }

        if (max_amp == 0.0) return 0.0;
        return clampF64(total / max_amp, 0.0, 1.0);
    }

    /// Generates a row-major `width * height` 2D heightmap normalized to `[0.0, 1.0]`.
    /// Caller owns the returned slice and must free it with `alloc.free`.
    fn generateGrid2D(
        alloc: std.mem.Allocator,
        width: usize,
        height: usize,
        x0: f64,
        y0: f64,
        step: f64,
        cfg: FractalConfig
    ): []f64 ! NoiseError {
        if (width == 0 || height == 0) return NoiseError.InvalidDimensions;
        if (height > std.math.maxInt(usize) / width) return NoiseError.InvalidDimensions;
        if (cfg.octaves == 0) return NoiseError.InvalidOctaves;
        if (cfg.frequency <= 0.0) return NoiseError.InvalidFrequency;

        const total_len = width * height;
        let grid = alloc.alloc(f64, total_len) catch return NoiseError.OutOfMemory;

        let row: usize = 0;
        while (row < height) : (row += 1) {
            const ny = y0 + f64(row) * step;
            let col: usize = 0;
            while (col < width) : (col += 1) {
                const nx = x0 + f64(col) * step;
                const v = _.fbm2(nx, ny, cfg) catch e => {
                    alloc.free(grid);
                    return e;
                };
                grid[row * width + col] = clampF64((v + 1.0) * 0.5, 0.0, 1.0);
            }
        }

        return grid;
    }
}
