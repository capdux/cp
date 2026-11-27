// Type definitions for CP

import { Xoshiro256, nextF64 } from "../../xoshiro256/src/index.cp";
export fn rdstAbs(x: f64): f64;
export fn rdstSqrt(x: f64): f64;
export fn rdstLn(x: f64): f64;
export fn rdstCos(angle: f64): f64;
export fn rdstOpen01(rng: &mut Xoshiro256): f64;
export class UniformRange {
    low: f64,
    high: f64,
    fn new(low: f64, high: f64): UniformRange?;
    fn sample(rng: &mut Xoshiro256): f64;
}

export class NormalDistr {
    mean: f64,
    std_dev: f64,
    fn new(mean: f64, std_dev: f64): NormalDistr?;
    fn sample(rng: &mut Xoshiro256): f64;
}

export class ExpDistr {
    lambda: f64,
    fn new(lambda: f64): ExpDistr?;
    fn sample(rng: &mut Xoshiro256): f64;
}

export class BernoulliDistr {
    p: f64,
    fn new(p: f64): BernoulliDistr?;
    fn sample(rng: &mut Xoshiro256): bool;
}

export class WeightedIndex {
    cumulative: [16]f64,
    count: usize,
    total: f64,
    fn fromWeights(weights: []f64): WeightedIndex?;
    fn sample(rng: &mut Xoshiro256): usize;
}

