// Type definitions for CP

export class PemBlock {
    label: string,
    body_b64: string,
    next_pos: usize,
    valid: bool,
}

export fn bytesEq(a: string, b: string): bool;
export fn startsWith(s: string, prefix: string): bool;
export fn endsWith(s: string, suffix: string): bool;
export fn isValidLabel(label: string): bool;
export fn nextBlock(input: string, start: usize): PemBlock;
export fn countBlocks(input: string): usize;
export fn isB64Ws(c: u8): bool;
export fn b64DecVal(c: u8): i32;
export fn b64EncChar(v: u8): u8;
export fn decodeBase64Body(body_b64: string, out_der: []u8): i64;
export fn appendSlice(out: []u8, pos: usize, s: string): i64;
export fn appendB64Wrapped(out: []u8, pos: usize, col: &mut usize, c: u8): i64;
export fn encodePem(label: string, der: &[]u8, out: []u8): usize;
