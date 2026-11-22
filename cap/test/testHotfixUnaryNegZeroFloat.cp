import { print } from "cap:io";

fn check(cond: bool, msg: string) {
    if (!cond) {
        @panic(msg);
    }
}

fn neg64(x: f64): f64 {
    return -x;
}

fn neg32(x: f32): f32 {
    return -x;
}

fn getZero64(): f64 {
    return 0.0;
}

fn negInt(x: i32): i32 {
    return -x;
}

fn main() {
    let SIGN64: u64 = (1 as u64) << 63;
    let SIGN32: u32 = (1 as u32) << 31;

    // 1. Direct float literal -0.0 (f64 / f32)
    let lit64: f64 = -0.0;
    let lit64_bits: u64 = @bitCast(lit64);
    check(lit64_bits == SIGN64, "lit64 -0.0 sign bit");

    let lit32: f32 = -0.0;
    let lit32_bits: u32 = @bitCast(lit32);
    check(lit32_bits == SIGN32, "lit32 -0.0 sign bit");

    // 2. Parenthesized -(0.0)
    let paren64: f64 = -(0.0);
    let paren64_bits: u64 = @bitCast(paren64);
    check(paren64_bits == SIGN64, "paren64 -(0.0) sign bit");

    let paren32: f32 = -(0.0);
    let paren32_bits: u32 = @bitCast(paren32);
    check(paren32_bits == SIGN32, "paren32 -(0.0) sign bit");

    // 3. Variable negation: let z: f64 = 0.0; -z
    let z64: f64 = 0.0;
    let nz64: f64 = -z64;
    let nz64_bits: u64 = @bitCast(nz64);
    check(nz64_bits == SIGN64, "var -z64 sign bit");

    let z32: f32 = 0.0;
    let nz32: f32 = -z32;
    let nz32_bits: u32 = @bitCast(nz32);
    check(nz32_bits == SIGN32, "var -z32 sign bit");

    // 4. Function parameter & call negation
    let fn_nz64: f64 = neg64(0.0);
    let fn_nz64_bits: u64 = @bitCast(fn_nz64);
    check(fn_nz64_bits == SIGN64, "fn neg64(0.0) sign bit");

    let fn_nz32: f32 = neg32(0.0);
    let fn_nz32_bits: u32 = @bitCast(fn_nz32);
    check(fn_nz32_bits == SIGN32, "fn neg32(0.0) sign bit");

    let call_nz64: f64 = -getZero64();
    let call_nz64_bits: u64 = @bitCast(call_nz64);
    check(call_nz64_bits == SIGN64, "call -getZero64() sign bit");

    // 5. Double negation -(-0.0) and -(-z) restores +0.0
    let dbl_lit64: f64 = -(-0.0);
    let dbl_lit64_bits: u64 = @bitCast(dbl_lit64);
    check(dbl_lit64_bits == 0, "double neg -(-0.0) restores +0.0");

    let dbl_var64: f64 = -(-z64);
    let dbl_var64_bits: u64 = @bitCast(dbl_var64);
    check(dbl_var64_bits == 0, "double neg -(-z64) restores +0.0");

    // 6. Explicit subtraction 0.0 - z64 remains +0.0 per IEEE-754
    let sub_z64: f64 = 0.0 - z64;
    let sub_z64_bits: u64 = @bitCast(sub_z64);
    check(sub_z64_bits == 0, "explicit 0.0 - z64 remains +0.0");

    // 7. Signed integers, precedence -(a + b), and i32 min boundary
    let a: i32 = 10;
    let b: i32 = 32;
    check(-1 == -1, "-1 == -1");
    check(-a == -10, "-a == -10");
    check(negInt(42) == -42, "negInt(42) == -42");
    check(-(a + b) == -42, "-(a + b) == -42");
    check(-(-a) == 10, "-(-a) == 10");
    let min_i32: i32 = -2147483648;
    check(min_i32 < 0, "min_i32 < 0");

    print("testHotfixUnaryNegZeroFloat PASS");
}
