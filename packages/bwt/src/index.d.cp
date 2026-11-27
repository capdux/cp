// Type definitions for CP

export class BwtResult {
    primary_index: usize,
    len: usize,
    ok: bool,
}

export fn cmpRotations(src: &[]u8, a: usize, b: usize): i32;
export fn bwtForward(src: &[]u8, dst: []u8): BwtResult;
export fn bwtInverse(src: &[]u8, primary_index: usize, dst: []u8): bool;
export fn mtfEncode(src: &[]u8, dst: []u8): bool;
export fn mtfDecode(src: &[]u8, dst: []u8): bool;
