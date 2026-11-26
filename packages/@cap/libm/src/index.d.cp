// Type definitions for CP

export fn lmNan(): f64;
export fn lmInf(): f64;
export fn lmScalePow2(val: f64, exp2: i64): f64;
export fn fabs(x: f64): f64;
export fn trunc(x: f64): f64;
export fn floor(x: f64): f64;
export fn ceil(x: f64): f64;
export fn round(x: f64): f64;
export fn fmod(x: f64, y: f64): f64;
export fn copysign(x: f64, y: f64): f64;
export fn fmin(x: f64, y: f64): f64;
export fn fmax(x: f64, y: f64): f64;
export fn sqrt(x: f64): f64;
export fn cbrt(x: f64): f64;
export fn hypot(x: f64, y: f64): f64;
export fn exp(x: f64): f64;
export fn ln(x: f64): f64;
export fn log2(x: f64): f64;
export fn log10(x: f64): f64;
export fn pow(x: f64, y: f64): f64;
export fn lmSinCore(r: f64): f64;
export fn lmCosCore(r: f64): f64;
export class LmTrigRed {
    r: f64,
    quad: i64,
}

export fn lmReduceTrig(x: f64): LmTrigRed;
export fn sin(x: f64): f64;
export fn cos(x: f64): f64;
export fn tan(x: f64): f64;
export fn lmAtanUnit(u: f64): f64;
export fn atan(x: f64): f64;
export fn atan2(y: f64, x: f64): f64;
export fn asin(x: f64): f64;
export fn acos(x: f64): f64;
export fn sinh(x: f64): f64;
export fn cosh(x: f64): f64;
export fn tanh(x: f64): f64;
