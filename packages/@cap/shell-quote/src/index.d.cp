// Type definitions for CP

export fn isSafeBareChar(c: u8): bool;
export fn needsQuoting(arg: &[]u8): bool;
export fn quoteArg(arg: &[]u8, dst: []u8): i64;
export fn quoteArgs(args: &[]&[]u8, dst: []u8): i64;
export class ParsedArgSpan {
    start: usize,
    len: usize,
}

export class SplitResult {
    arg_count: usize,
    bytes_used: usize,
    ok: bool,
}

export fn splitCommand(cmd: &[]u8, out_buf: []u8, out_spans: []ParsedArgSpan): SplitResult;
