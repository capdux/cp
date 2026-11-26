// Type definitions for CP

import { gcdU64, divFloor, divCeil } from "../../num-integer/src/index.cp";
export fn nratAbsU64(v: i64): u64;
export fn nratReduce(numer: i64, denom: i64): Ratio64;
export class Ratio64 {
    numer: i64,
    denom: i64,
    fn new(numer: i64, denom: i64): Ratio64?;
    fn fromInt(n: i64): Ratio64;
    fn zero(): Ratio64;
    fn one(): Ratio64;
    fn isInteger(): bool;
    fn isZero(): bool;
    fn isPositive(): bool;
    fn isNegative(): bool;
    fn add(other: Ratio64): Ratio64;
    fn sub(other: Ratio64): Ratio64;
    fn mul(other: Ratio64): Ratio64;
    fn recip(): Ratio64?;
    fn div(other: Ratio64): Ratio64?;
    fn neg(): Ratio64;
    fn abs(): Ratio64;
    fn cmp(other: Ratio64): i32;
    fn eq(other: Ratio64): bool;
    fn floor(): i64;
    fn ceil(): i64;
    fn toF64(): f64;
}

