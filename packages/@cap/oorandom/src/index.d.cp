// Type definitions for CP

export fn oorStep(st: u64, inc: u64): u64;
export fn oorOutput(oldstate: u64): u32;
export class Rand32 {
    st: u64,
    inc: u64,
    fn newInc(seed: u64, inc: u64): Rand32;
    fn new(seed: u64): Rand32;
    fn randU32(_: auto): u32;
    fn randI32(_: auto): i32;
    fn randFloat(_: auto): f32;
    fn randRange(_: auto, start: u32, end: u32): u32;
    fn state(): u64;
}

export class Rand64 {
    lo: Rand32,
    hi: Rand32,
    fn new(seed: u64): Rand64;
    fn randU64(_: auto): u64;
    fn randFloat(_: auto): f64;
}

