// Type definitions for CP

export enum ShlexError {
    None,
    UnclosedSingleQuote,
    UnclosedDoubleQuote,
    TrailingBackslash,
    TooManyTokens,
    BufferOverflow
}

export class SplitResult {
    pool: [MAX_POOL_BYTES]u8,
    starts: [MAX_TOKENS]usize,
    lens: [MAX_TOKENS]usize,
    count: usize,
    ok: bool,
    err: ShlexError,
    fn empty(): SplitResult;
    fn tokenAt(idx: usize): string;
}

export fn isSafeUnquotedChar(c: u8): bool;
export fn isSpace(c: u8): bool;
export fn needsQuote(arg: string): bool;
export fn hasShellMetachars(s: string): bool;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn quoteArg(arg: string, out: []u8): string;
export fn joinArgs(args: []string, out: []u8): string;
export fn splitShell(cmd: string): SplitResult;
