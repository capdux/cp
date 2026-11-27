// Type definitions for CP

export class AnsiToken {
    kind: u8,
    start: usize,
    len: usize,
    final_byte: u8,
}

export class SgrParams {
    params: [16]u16,
    count: usize,
    valid: bool,
}

export fn nextToken(input: string, pos: usize): AnsiToken;
export fn tokenizeAnsi(input: string, out: []AnsiToken): usize;
export fn parseCsiParams(csi_seq: string): SgrParams;
export fn stripAnsiToBuf(input: string, out: []u8): usize;
export fn hasAnsi(input: string): bool;
export fn visibleByteLen(input: string): usize;
