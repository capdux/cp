// Type definitions for CP

export class Utf8Error {
    valid_up_to: usize,
    error_len: u8,
}

export class Utf8CheckResult {
    valid: bool,
    valid_up_to: usize,
    error_len: u8,
    codepoint_count: usize,
}

export class StepResult {
    valid: bool,
    step_len: usize,
    error_len: u8,
}

export fn isCont(b: u8): bool;
export fn checkSeqAt(data: string, pos: usize): StepResult;
export fn isAscii(data: string): bool;
export fn validateUtf8(data: string): Utf8CheckResult;
export fn isValidUtf8(data: string): bool;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn toLossyUtf8(data: string, out: []u8): string;
export fn utf16Len(data: string): usize;
