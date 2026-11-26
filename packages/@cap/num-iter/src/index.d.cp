// Type definitions for CP

import { divCeil, divFloor } from "../../num-integer/src/index.cp";
export fn nitrAbsI64(v: i64): u64;
export fn nitrCheckedAddI64(a: i64, b: i64): i64?;
export fn nitrDiffU64(hi: i64, lo: i64): u64;
export fn nitrAdvanceSigned(state: i64, step: i64, n: usize): i64;
export class RangeStep {
    state: i64,
    stop: i64,
    step: i64,
    done: bool,
    fn isDone(): bool;
    fn remaining(): usize;
    fn next(_: auto): i64?;
    fn nth(_: auto, n: usize): i64?;
}

export class RangeStepInclusive {
    state: i64,
    stop: i64,
    step: i64,
    done: bool,
    fn isDone(): bool;
    fn remaining(): usize;
    fn next(_: auto): i64?;
}

export class RangeStepU64 {
    state: u64,
    stop: u64,
    step: u64,
    done: bool,
    fn isDone(): bool;
    fn remaining(): usize;
    fn next(_: auto): u64?;
    fn nth(_: auto, n: usize): u64?;
}

export fn rangeStep(start: i64, stop: i64, step: i64): RangeStep;
export fn rangeStepInclusive(start: i64, stop: i64, step: i64): RangeStepInclusive;
export fn rangeStepU64(start: u64, stop: u64, step: u64): RangeStepU64;
export fn collectRangeStep(r: &mut RangeStep, out: []i64): usize;
