// Type definitions for CP

export class ExtentF64 {
    valid: bool,
    min: f64,
    max: f64,
}

export fn d3aIsNan(v: f64): bool;
export fn sumF64(data: &[]f64): f64;
export fn meanF64(data: &[]f64): f64?;
export fn extentF64(data: &[]f64): ExtentF64;
export fn varianceF64(data: &[]f64): f64?;
export fn deviationF64(data: &[]f64): f64?;
export fn quantileSorted(sorted: &[]f64, p: f64): f64?;
export fn medianSorted(sorted: &[]f64): f64?;
export fn bisectLeft(sorted: &[]f64, x: f64): usize;
export fn bisectRight(sorted: &[]f64, x: f64): usize;
export fn cumsumF64(data: &[]f64, out: []f64): usize;
export fn binCounts(data: &[]f64, domain_min: f64, domain_max: f64, out_counts: []usize): usize;
