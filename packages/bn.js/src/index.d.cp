// Type definitions for CP

export class Add64Res {
    sum: u64,
    carry: u64,
}

export class Sub64Res {
    diff: u64,
    borrow: u64,
}

export class Mul64Res {
    lo: u64,
    hi: u64,
}

export fn addWithCarry64(a: u64, b: u64, carry_in: u64): Add64Res;
export fn subWithBorrow64(a: u64, b: u64, borrow_in: u64): Sub64Res;
export fn mulWide64(a: u64, b: u64): Mul64Res;
export fn clz64(x: u64): u32;
export fn hexVal(c: u8): i32;
export fn hexDigit(v: u8): u8;
export class DivRem256 {
    quot: U256,
    rem: U256,
}

export class U256 {
    limbs: [4]u64,
    fn zero(): U256;
    fn one(): U256;
    fn fromU64(v: u64): U256;
    fn fromU128(lo: u64, hi: u64): U256;
    fn maxVal(): U256;
    fn fromHex(s: string): U256;
    fn isZero(): bool;
    fn cmp(other: U256): i32;
    fn eq(other: U256): bool;
    fn wrappingAdd(other: U256): U256;
    fn wrappingSub(other: U256): U256;
    fn wrappingMul(other: U256): U256;
    fn shl(bits: u32): U256;
    fn shr(bits: u32): U256;
    fn bitAnd(other: U256): U256;
    fn bitOr(other: U256): U256;
    fn bitXor(other: U256): U256;
    fn leadingZeros(): u32;
    fn bitLen(): u32;
    fn divRem(divisor: U256): DivRem256;
    fn toHex(out: []u8): usize;
}

export fn zero(): U256;
export fn one(): U256;
export fn fromU64(v: u64): U256;
export fn fromU128(lo: u64, hi: u64): U256;
export fn maxVal(): U256;
export fn fromHex(s: string): U256;
