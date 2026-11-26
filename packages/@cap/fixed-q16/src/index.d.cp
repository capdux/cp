// Type definitions for CP

export class Fixed16 {
    raw: i32,
    fn fromRaw(raw: i32): Fixed16;
    fn fromInt(v: i32): Fixed16;
    fn fromF64(v: f64): Fixed16;
    fn zero(): Fixed16;
    fn one(): Fixed16;
    fn toF64(): f64;
    fn floorInt(): i32;
    fn ceilInt(): i32;
    fn roundInt(): i32;
    fn add(other: Fixed16): Fixed16;
    fn sub(other: Fixed16): Fixed16;
    fn neg(): Fixed16;
    fn abs(): Fixed16;
    fn mul(other: Fixed16): Fixed16;
    fn div(other: Fixed16): Fixed16;
    fn clamp(lo: Fixed16, hi: Fixed16): Fixed16;
    fn lerp(other: Fixed16, t: Fixed16): Fixed16;
}

