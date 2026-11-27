// Type definitions for CP

export fn oorStep(st: u64, inc: u64): u64;
export fn oorOutput(oldstate: u64): u32;
export fn oorAdvanceState(st_in: u64, inc: u64, delta_in: u64): u64;
export class Rand32 {
    st: u64,
    inc: u64,
    fn newInc(seed: u64, inc: u64): Rand32;
    fn new(seed: u64): Rand32;
    fn fromState(st: u64, inc: u64): Rand32;
    fn randU32(_: auto): u32;
    fn randI32(_: auto): i32;
    fn randFloat(_: auto): f32;
    fn randBool(_: auto): bool;
    fn randRange(_: auto, start: u32, end: u32): u32;
    fn randRangeI32(_: auto, start: i32, end: i32): i32;
    fn fillBytes(_: auto, dst: []u8);
    fn advance(_: auto, delta: u64);
    fn state(): u64;
    fn statePair(): [2]u64;
}

export class Rand64 {
    lo: Rand32,
    hi: Rand32,
    fn new(seed: u64): Rand64;
    fn newInc(seed_lo: u64, seed_hi: u64, inc_lo: u64, inc_hi: u64): Rand64;
    fn randU64(_: auto): u64;
    fn randI64(_: auto): i64;
    fn randFloat(_: auto): f64;
    fn randBool(_: auto): bool;
    fn randRange(_: auto, start: u64, end: u64): u64;
    fn randRangeI64(_: auto, start: i64, end: i64): i64;
    fn fillBytes(_: auto, dst: []u8);
    fn advance(_: auto, delta: u64);
    fn state(): [2]u64;
}

