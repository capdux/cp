// Type definitions for CP

export fn absU64(n: i64): u64;
export fn divTruncPos(a: i64, b_pos: i64): i64;
export fn remMagPos(a: i64, b_pos: i64): u64;
export fn gcdU64(a: u64, b: u64): u64;
export fn isWs(c: u8): bool;
export fn trimWs(s: string): string;
export fn parseI64Strict(raw: string): i64?;
export class Ratio64 {
    numer: i64,
    denom: i64,
    fn new(numer: i64, denom: i64): Ratio64?;
    fn fromInt(n: i64): Ratio64;
    fn zero(): Ratio64;
    fn one(): Ratio64;
    fn neg(): Ratio64;
    fn abs(): Ratio64;
    fn recip(): Ratio64?;
    fn add(other: Ratio64): Ratio64;
    fn sub(other: Ratio64): Ratio64;
    fn mul(other: Ratio64): Ratio64;
    fn div(other: Ratio64): Ratio64?;
    fn equals(other: Ratio64): bool;
    fn isInteger(): bool;
    fn trunc(): i64;
    fn floor(): i64;
    fn ceil(): i64;
    fn cmp(other: Ratio64): i32;
    fn toF64(): f64;
}

export fn parseRatio(s: string): Ratio64?;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendI64(out: []u8, start_pos: usize, val: i64): usize;
export fn formatRatio(r: Ratio64, out: []u8): string;
