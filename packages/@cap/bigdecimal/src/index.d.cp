// Type definitions for CP

export fn bdecIsSpace(c: u8): bool;
export fn bdecIsDigit(c: u8): bool;
export fn bdecAbsU64(v: i64): u64;
export fn bdecSignedFromU64(mag: u64, neg: bool): i64;
export fn bdecPow10U64(exp: u32): u64;
export fn bdecDigitCount(u: u64): i32;
export fn bdecWriteByte(out: []u8, pos: usize, b: u8): usize;
export class BigDecimal {
    digits: i64,
    scale: i32,
    fn zero(): BigDecimal;
    fn fromInt(v: i64): BigDecimal;
    fn new(digits: i64, scale: i32): BigDecimal;
    fn normalize(): BigDecimal;
    fn neg(): BigDecimal;
    fn abs(): BigDecimal;
    fn round(new_scale: i32): BigDecimal;
    fn cmp(other: BigDecimal): i32;
    fn eq(other: BigDecimal): bool;
    fn add(other: BigDecimal): BigDecimal;
    fn sub(other: BigDecimal): BigDecimal;
    fn mul(other: BigDecimal): BigDecimal;
    fn divWithScale(other: BigDecimal, target_scale: i32): BigDecimal?;
}

export fn parseDecimal(s: string): BigDecimal?;
export fn formatDecimal(val: BigDecimal, out: []u8): usize;
