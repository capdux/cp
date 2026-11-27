// Type definitions for CP

export fn absU64(v: i64): u64;
export fn divShift32U64(a: u64, b: u64): u64;
export class Fixed32 {
    raw: i64,
    fn fromInt(v: i32): Fixed32;
    fn fromRaw(raw: i64): Fixed32;
    fn fromRatio(num: i64, den: i64): Fixed32;
    fn fromF64(v: f64): Fixed32;
    fn rawValue(): i64;
    fn toF64(): f64;
    fn toInt(): i32;
    fn floor(): i32;
    fn ceil(): i32;
    fn round(): i32;
    fn add(other: Fixed32): Fixed32;
    fn sub(other: Fixed32): Fixed32;
    fn neg(): Fixed32;
    fn abs(): Fixed32;
    fn mul(other: Fixed32): Fixed32;
    fn div(other: Fixed32): Fixed32;
    fn lerp(other: Fixed32, t: Fixed32): Fixed32;
    fn clamp(lo: Fixed32, hi: Fixed32): Fixed32;
    fn sqrt(): Fixed32;
    fn cmp(other: Fixed32): i32;
}

