// @cap/noise — Zero-heap Improved Perlin Noise (2D/3D) & Fractal Brownian Motion (fBm)
// Clean-room MIT implementation in pure CP (no heap allocations, no FFI).

fn floorInt(x: f64): i64 {
    let i: i64 = x as i64;
    if ((i as f64) > x) {
        return i - 1;
    }
    return i;
}

fn fade(t: f64): f64 {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

fn lerp(a: f64, b: f64, t: f64): f64 {
    return a + t * (b - a);
}

fn grad2(hash: u8, x: f64, y: f64): f64 {
    let h: u8 = hash & 7;
    if (h == 0) { return x + y; }
    if (h == 1) { return -x + y; }
    if (h == 2) { return x - y; }
    if (h == 3) { return -x - y; }
    if (h == 4) { return x; }
    if (h == 5) { return -x; }
    if (h == 6) { return y; }
    return -y;
}

fn grad3(hash: u8, x: f64, y: f64, z: f64): f64 {
    let h: u8 = hash & 15;
    let u: f64 = x;
    if (h >= 8) {
        u = y;
    }
    let v: f64 = y;
    if (h >= 4) {
        if (h == 12 || h == 14) {
            v = x;
        } else {
            v = z;
        }
    }
    let res_u: f64 = u;
    if ((h & 1) != 0) {
        res_u = -u;
    }
    let res_v: f64 = v;
    if ((h & 2) != 0) {
        res_v = -v;
    }
    return res_u + res_v;
}

fn splitmix64(state: *u64): u64 {
    let c1: u64 = ((0x9E3779B9 as u64) << 32) | (0x7F4A7C15 as u64);
    let c2: u64 = ((0xBF58476D as u64) << 32) | (0x1CE4E5B9 as u64);
    let c3: u64 = ((0x94D049BB as u64) << 32) | (0x133111EB as u64);
    state.* = state.* +% c1;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% c2;
    z = (z ^ (z >> 27)) *% c3;
    return z ^ (z >> 31);
}

pub struct PerlinNoise {
    perm: [512]u8,

    pub static fn init(seed: u64): PerlinNoise {
        let p: [256]u8 = undefined;
        let i: usize = 0;
        while (i < 256) {
            p[i] = i as u8;
            i += 1;
        }
        let rng: u64 = seed;
        let j: usize = 255;
        while (j > 0) {
            let r: usize = (splitmix64(&rng) % ((j + 1) as u64)) as usize;
            let tmp: u8 = p[j];
            p[j] = p[r];
            p[r] = tmp;
            j -= 1;
        }
        let perm: [512]u8 = undefined;
        let k: usize = 0;
        while (k < 256) {
            perm[k] = p[k];
            perm[k + 256] = p[k];
            k += 1;
        }
        return PerlinNoise { perm: perm };
    }

    pub fn noise2D(x: f64, y: f64): f64 {
        let xi_i: i64 = floorInt(x);
        let yi_i: i64 = floorInt(y);
        let xf: f64 = x - (xi_i as f64);
        let yf: f64 = y - (yi_i as f64);
        let xi: usize = (xi_i & 255) as usize;
        let yi: usize = (yi_i & 255) as usize;

        let u: f64 = fade(xf);
        let v: f64 = fade(yf);

        let aa: u8 = _.perm[(_.perm[xi] as usize) + yi];
        let ab: u8 = _.perm[(_.perm[xi] as usize) + yi + 1];
        let ba: u8 = _.perm[(_.perm[xi + 1] as usize) + yi];
        let bb: u8 = _.perm[(_.perm[xi + 1] as usize) + yi + 1];

        let x1: f64 = lerp(grad2(aa, xf, yf), grad2(ba, xf - 1.0, yf), u);
        let x2: f64 = lerp(grad2(ab, xf, yf - 1.0), grad2(bb, xf - 1.0, yf - 1.0), u);
        return lerp(x1, x2, v);
    }

    pub fn noise3D(x: f64, y: f64, z: f64): f64 {
        let xi_i: i64 = floorInt(x);
        let yi_i: i64 = floorInt(y);
        let zi_i: i64 = floorInt(z);
        let xf: f64 = x - (xi_i as f64);
        let yf: f64 = y - (yi_i as f64);
        let zf: f64 = z - (zi_i as f64);
        let xi: usize = (xi_i & 255) as usize;
        let yi: usize = (yi_i & 255) as usize;
        let zi: usize = (zi_i & 255) as usize;

        let u: f64 = fade(xf);
        let v: f64 = fade(yf);
        let w: f64 = fade(zf);

        let a: usize = (_.perm[xi] as usize) + yi;
        let aa: usize = (_.perm[a] as usize) + zi;
        let ab: usize = (_.perm[a + 1] as usize) + zi;
        let b: usize = (_.perm[xi + 1] as usize) + yi;
        let ba: usize = (_.perm[b] as usize) + zi;
        let bb: usize = (_.perm[b + 1] as usize) + zi;

        let x1: f64 = lerp(grad3(_.perm[aa], xf, yf, zf), grad3(_.perm[ba], xf - 1.0, yf, zf), u);
        let x2: f64 = lerp(grad3(_.perm[ab], xf, yf - 1.0, zf), grad3(_.perm[bb], xf - 1.0, yf - 1.0, zf), u);
        let y1: f64 = lerp(x1, x2, v);

        let x3: f64 = lerp(grad3(_.perm[aa + 1], xf, yf, zf - 1.0), grad3(_.perm[ba + 1], xf - 1.0, yf, zf - 1.0), u);
        let x4: f64 = lerp(grad3(_.perm[ab + 1], xf, yf - 1.0, zf - 1.0), grad3(_.perm[bb + 1], xf - 1.0, yf - 1.0, zf - 1.0), u);
        let y2: f64 = lerp(x3, x4, v);

        return lerp(y1, y2, w);
    }

    // Fractal Brownian Motion (fBm) in 2D, normalized to approximately [-1, 1]
    pub fn fbm2D(x: f64, y: f64, octaves: u32, lacunarity: f64, gain: f64): f64 {
        if (octaves == 0) {
            return 0.0;
        }
        let sum: f64 = 0.0;
        let amp: f64 = 1.0;
        let freq: f64 = 1.0;
        let max_amp: f64 = 0.0;
        let i: u32 = 0;
        while (i < octaves) {
            sum = sum + _.noise2D(x * freq, y * freq) * amp;
            max_amp = max_amp + amp;
            amp = amp * gain;
            freq = freq * lacunarity;
            i += 1;
        }
        return sum / max_amp;
    }

    // Ridged multifractal 2D noise in [0, 1] (ideal for mountain ridges and clouds)
    pub fn ridged2D(x: f64, y: f64, octaves: u32): f64 {
        if (octaves == 0) {
            return 0.0;
        }
        let sum: f64 = 0.0;
        let amp: f64 = 1.0;
        let freq: f64 = 1.0;
        let max_amp: f64 = 0.0;
        let i: u32 = 0;
        while (i < octaves) {
            let n: f64 = _.noise2D(x * freq, y * freq);
            if (n < 0.0) {
                n = -n;
            }
            let ridge: f64 = 1.0 - n;
            sum = sum + ridge * ridge * amp;
            max_amp = max_amp + amp;
            amp = amp * 0.5;
            freq = freq * 2.0;
            i += 1;
        }
        return sum / max_amp;
    }
}
