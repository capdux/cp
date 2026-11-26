// Type definitions for CP

import { fnv1a64 } from "../../fnv-hash/src/index.cp";
export fn isNanF64(x: f64): bool;
export fn isInfiniteF64(x: f64): bool;
export fn isFiniteF64(x: f64): bool;
export fn cmpOrderedF64(a: f64, b: f64): i32;
export fn eqOrderedF64(a: f64, b: f64): bool;
export fn hashOrderedF64(x: f64): u64;
export class OrderedF64 {
    val: f64,
    fn new(v: f64): OrderedF64;
    fn intoInner(): f64;
    fn isNan(): bool;
    fn isFinite(): bool;
    fn cmp(other: OrderedF64): i32;
    fn eq(other: OrderedF64): bool;
    fn hash(): u64;
    fn min(other: OrderedF64): OrderedF64;
    fn max(other: OrderedF64): OrderedF64;
    fn clamp(lo: OrderedF64, hi: OrderedF64): OrderedF64;
}

export class NotNanF64 {
    val: f64,
    fn new(v: f64): NotNanF64?;
    fn intoInner(): f64;
    fn cmp(other: NotNanF64): i32;
    fn eq(other: NotNanF64): bool;
    fn hash(): u64;
}

export fn sortOrderedF64(arr: []f64);
