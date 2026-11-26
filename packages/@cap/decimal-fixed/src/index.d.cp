// Type definitions for CP

export fn absToU64(v: i64): u64;
export fn clampScale(s: u8): u8;
export fn pow10F64(s: u8): f64;
export class ParseDecResult {
    value: Decimal,
    valid: bool,
}

export class Decimal {
    mantissa: i64,
    scale: u8,
    fn new(mantissa: i64, scale: u8): Decimal;
    fn fromInt(v: i64): Decimal;
    fn zero(): Decimal;
    fn one(): Decimal;
    fn parse(s: string): ParseDecResult;
    fn normalize(): Decimal;
    fn rescale(new_scale: u8): Decimal;
    fn roundHalfEven(dp: u8): Decimal;
    fn neg(): Decimal;
    fn abs(): Decimal;
    fn add(other: Decimal): Decimal;
    fn sub(other: Decimal): Decimal;
    fn mul(other: Decimal): Decimal;
    fn div(other: Decimal, out_scale: u8): Decimal;
    fn cmp(other: Decimal): i32;
    fn eq(other: Decimal): bool;
    fn toF64(): f64;
    fn format(out: []u8): usize;
}

