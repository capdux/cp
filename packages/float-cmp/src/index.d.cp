// Type definitions for CP

import { F64_EPSILON, F32_EPSILON, absDiffEqF64, absDiffEqF32, relativeEqF64, relativeEqF32, ulpsDistanceF64, ulpsEqF64, ulpsDistanceF32, ulpsEqF32 } from "../../approx/src/index.cp";
export fn fcmpIsNanF64(x: f64): bool;
export fn fcmpIsNanF32(x: f32): bool;
export fn fcmpIsInfF64(x: f64): bool;
export fn fcmpIsInfF32(x: f32): bool;
export fn f64ToOrderedI64(x: f64): i64;
export fn f32ToOrderedI32(x: f32): i32;
export class F64Margin {
    epsilon: f64,
    ulps: u64,
    fn defaultMargin(): F64Margin;
    fn zero(): F64Margin;
    fn fromTuple(epsilon: f64, ulps: u64): F64Margin;
    fn withEpsilon(eps: f64): F64Margin;
    fn withUlps(u: u64): F64Margin;
}

export class F32Margin {
    epsilon: f32,
    ulps: u32,
    fn defaultMargin(): F32Margin;
    fn zero(): F32Margin;
    fn fromTuple(epsilon: f32, ulps: u32): F32Margin;
    fn withEpsilon(eps: f32): F32Margin;
    fn withUlps(u: u32): F32Margin;
}

export fn ulpsDiffF64(a: f64, b: f64): i64?;
export fn ulpsDiffF32(a: f32, b: f32): i32?;
export fn nextUpF64(x: f64): f64;
export fn nextDownF64(x: f64): f64;
export fn nextUp(x: f64): f64;
export fn nextDown(x: f64): f64;
export fn nextUpF32(x: f32): f32;
export fn nextDownF32(x: f32): f32;
export fn approxEqF64(a: f64, b: f64, margin: F64Margin): bool;
export fn approxNeF64(a: f64, b: f64, margin: F64Margin): bool;
export fn approxEqF32(a: f32, b: f32, margin: F32Margin): bool;
export fn approxNeF32(a: f32, b: f32, margin: F32Margin): bool;
export fn approxEqRatioF64(a: f64, b: f64, ratio: f64): bool;
export fn approxEqRatioF32(a: f32, b: f32, ratio: f32): bool;
export fn approxOrdF64(a: f64, b: f64, margin: F64Margin): i32;
export fn approxOrdF32(a: f32, b: f32, margin: F32Margin): i32;
