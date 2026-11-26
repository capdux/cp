// Type definitions for CP

export fn floorInt(x: f64): i64;
export fn fade(t: f64): f64;
export fn lerp(a: f64, b: f64, t: f64): f64;
export fn grad2(hash: u8, x: f64, y: f64): f64;
export fn grad3(hash: u8, x: f64, y: f64, z: f64): f64;
export fn splitmix64(state: &mut u64): u64;
export class PerlinNoise {
    perm: [512]u8,
    fn init(seed: u64): PerlinNoise;
    fn noise2D(x: f64, y: f64): f64;
    fn noise3D(x: f64, y: f64, z: f64): f64;
    fn fbm2D(x: f64, y: f64, octaves: u32, lacunarity: f64, gain: f64): f64;
    fn ridged2D(x: f64, y: f64, octaves: u32): f64;
}

