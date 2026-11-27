// Type definitions for CP

export enum RoundingMode {
    HalfEven,
    HalfUp,
    HalfDown,
    Floor,
    Ceil,
    Trunc
}

export fn pow10(exp: u8): i128;
export fn absI128(v: i128): i128;
export fn divRoundI128(num: i128, den: i128, mode: RoundingMode): i128;
export class Decimal {
    mantissa: i128,
    scale: u8,
    fn zero(): Decimal;
    fn one(): Decimal;
    fn fromInt(v: i64): Decimal;
    fn fromParts(mantissa: i128, scale: u8): Decimal;
    fn parse(raw: string): Decimal?;
    fn isZero(): bool;
    fn isNegative(): bool;
    fn isPositive(): bool;
    fn neg(): Decimal;
    fn abs(): Decimal;
    fn normalize(): Decimal;
    fn round(dp: u8, mode: RoundingMode): Decimal;
    fn scaleTo(new_scale: u8, mode: RoundingMode): Decimal;
    fn cmp(other: Decimal): i8;
    fn eq(other: Decimal): bool;
    fn lt(other: Decimal): bool;
    fn lte(other: Decimal): bool;
    fn gt(other: Decimal): bool;
    fn gte(other: Decimal): bool;
    fn min(other: Decimal): Decimal;
    fn max(other: Decimal): Decimal;
    fn add(other: Decimal): Decimal;
    fn sub(other: Decimal): Decimal;
    fn mul(other: Decimal): Decimal;
    fn div(other: Decimal, dp: u8, mode: RoundingMode): Decimal?;
    fn format(out: []u8): string;
    fn toFixed(dp: u8, mode: RoundingMode, out: []u8): string;
}

