// @cap/perlin-noise — Zero-heap Ken Perlin improved 1D/2D/3D gradient noise & fBm in pure CP
// MIT License — Clean-room implementation using classic 256-entry permutation table

const PERM: [256]u8 = [
    151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225,
    140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
    247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
    57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
    74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122,
    60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
    65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
    200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64,
    52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212,
    207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213,
    119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
    129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
    218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,
    81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
    184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93,
    222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
];

fn floorF64(x: f64): i64 {
    const i: i64 = x as i64;
    return if (x < (i as f64)) i - 1 else i;
}

fn permAt(idx: usize, seed: u32): usize {
    const s0: usize = (seed & 255) as usize;
    const s1: usize = ((seed >> 8) & 255) as usize;
    const s2: usize = ((seed >> 16) & 255) as usize;
    const p1: usize = PERM[(idx ^ s0) & 255] as usize;
    const p2: usize = PERM[(p1 ^ s1) & 255] as usize;
    return (p2 ^ s2) & 255;
}

fn lerpF64(a: f64, b: f64, t: f64): f64 {
    return a + t * (b - a);
}

fn grad1D(hash: usize, x: f64): f64 {
    return if ((hash & 1) == 0) x else -x;
}

fn grad2D(hash: usize, x: f64, y: f64): f64 {
    const h: usize = hash & 7;
    if (h == 0) { return x + y; }
    if (h == 1) { return -x + y; }
    if (h == 2) { return x - y; }
    if (h == 3) { return -x - y; }
    if (h == 4) { return x; }
    if (h == 5) { return -x; }
    if (h == 6) { return y; }
    return -y;
}

fn grad3D(hash: usize, x: f64, y: f64, z: f64): f64 {
    const h: usize = hash & 15;
    const u: f64 = if (h < 8) x else y;
    const v: f64 = if (h < 4) y else if (h == 12 || h == 14) x else z;
    const su: f64 = if ((h & 1) == 0) u else -u;
    const sv: f64 = if ((h & 2) == 0) v else -v;
    return su + sv;
}

/// Ken Perlin's improved quintic fade curve: 6t^5 - 15t^4 + 10t^3.
pub fn fade(t: f64): f64 {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

/// 1D improved Perlin gradient noise in approximately [-1, 1].
pub fn noise1D(x: f64, seed: u32): f64 {
    const ix: i64 = floorF64(x);
    const xf: f64 = x - (ix as f64);
    const xi: usize = (ix & 255) as usize;
    const u: f64 = fade(xf);

    const a: usize = permAt(xi, seed);
    const b: usize = permAt((xi + 1) & 255, seed);

    const g0: f64 = grad1D(a, xf);
    const g1: f64 = grad1D(b, xf - 1.0);
    return 2.0 * lerpF64(g0, g1, u);
}

/// 2D improved Perlin gradient noise in approximately [-1, 1].
pub fn noise2D(x: f64, y: f64, seed: u32): f64 {
    const ix: i64 = floorF64(x);
    const iy: i64 = floorF64(y);
    const xf: f64 = x - (ix as f64);
    const yf: f64 = y - (iy as f64);

    const xi: usize = (ix & 255) as usize;
    const yi: usize = (iy & 255) as usize;
    const u: f64 = fade(xf);
    const v: f64 = fade(yf);

    const aa: usize = permAt((permAt(xi, seed) + yi) & 255, seed);
    const ab: usize = permAt((permAt(xi, seed) + yi + 1) & 255, seed);
    const ba: usize = permAt((permAt((xi + 1) & 255, seed) + yi) & 255, seed);
    const bb: usize = permAt((permAt((xi + 1) & 255, seed) + yi + 1) & 255, seed);

    const x1: f64 = lerpF64(grad2D(aa, xf, yf), grad2D(ba, xf - 1.0, yf), u);
    const x2: f64 = lerpF64(grad2D(ab, xf, yf - 1.0), grad2D(bb, xf - 1.0, yf - 1.0), u);
    return lerpF64(x1, x2, v);
}

/// 3D improved Perlin gradient noise in approximately [-1, 1].
pub fn noise3D(x: f64, y: f64, z: f64, seed: u32): f64 {
    const ix: i64 = floorF64(x);
    const iy: i64 = floorF64(y);
    const iz: i64 = floorF64(z);
    const xf: f64 = x - (ix as f64);
    const yf: f64 = y - (iy as f64);
    const zf: f64 = z - (iz as f64);

    const xi: usize = (ix & 255) as usize;
    const yi: usize = (iy & 255) as usize;
    const zi: usize = (iz & 255) as usize;
    const u: f64 = fade(xf);
    const v: f64 = fade(yf);
    const w: f64 = fade(zf);

    const a: usize = (permAt(xi, seed) + yi) & 255;
    const aa: usize = (permAt(a, seed) + zi) & 255;
    const ab: usize = (permAt((a + 1) & 255, seed) + zi) & 255;

    const b: usize = (permAt((xi + 1) & 255, seed) + yi) & 255;
    const ba: usize = (permAt(b, seed) + zi) & 255;
    const bb: usize = (permAt((b + 1) & 255, seed) + zi) & 255;

    const x1: f64 = lerpF64(
        grad3D(permAt(aa, seed), xf, yf, zf),
        grad3D(permAt(ba, seed), xf - 1.0, yf, zf),
        u
    );
    const x2: f64 = lerpF64(
        grad3D(permAt(ab, seed), xf, yf - 1.0, zf),
        grad3D(permAt(bb, seed), xf - 1.0, yf - 1.0, zf),
        u
    );
    const y1: f64 = lerpF64(x1, x2, v);

    const x3: f64 = lerpF64(
        grad3D(permAt((aa + 1) & 255, seed), xf, yf, zf - 1.0),
        grad3D(permAt((ba + 1) & 255, seed), xf - 1.0, yf, zf - 1.0),
        u
    );
    const x4: f64 = lerpF64(
        grad3D(permAt((ab + 1) & 255, seed), xf, yf - 1.0, zf - 1.0),
        grad3D(permAt((bb + 1) & 255, seed), xf - 1.0, yf - 1.0, zf - 1.0),
        u
    );
    const y2: f64 = lerpF64(x3, x4, v);
    return lerpF64(y1, y2, w);
}

/// 2D Fractal Brownian Motion (fBm), automatically normalized to approximately [-1, 1].
pub fn fbm2D(x: f64, y: f64, seed: u32, octaves: u32, lacunarity: f64, gain: f64): f64 {
    if (octaves == 0) {
        return 0.0;
    }
    let sum: f64 = 0.0;
    let norm: f64 = 0.0;
    let freq: f64 = 1.0;
    let amp: f64 = 1.0;
    let i: u32 = 0;
    while (i < octaves) {
        sum = sum + amp * noise2D(x * freq, y * freq, seed +% (i *% 1013));
        norm = norm + amp;
        freq = freq * lacunarity;
        amp = amp * gain;
        i += 1;
    }
    return if (norm > 0.0) sum / norm else 0.0;
}

/// 2D ridged multifractal noise (1 - |n|), normalized to approximately [0, 1].
pub fn ridge2D(x: f64, y: f64, seed: u32, octaves: u32): f64 {
    if (octaves == 0) {
        return 0.0;
    }
    let sum: f64 = 0.0;
    let norm: f64 = 0.0;
    let freq: f64 = 1.0;
    let amp: f64 = 1.0;
    let i: u32 = 0;
    while (i < octaves) {
        const n: f64 = noise2D(x * freq, y * freq, seed +% (i *% 1013));
        const an: f64 = if (n < 0.0) -n else n;
        let r: f64 = 1.0 - an;
        if (r < 0.0) {
            r = 0.0;
        }
        sum = sum + amp * r;
        norm = norm + amp;
        freq = freq * 2.0;
        amp = amp * 0.5;
        i += 1;
    }
    return if (norm > 0.0) sum / norm else 0.0;
}
