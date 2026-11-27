// Type definitions for CP

export fn fnmIsInvalidChar(ch: u8): bool;
export fn fnmHasInvalidChar(s: string): bool;
export fn fnmToLowerAscii(ch: u8): u8;
export fn fnmEq3IgnoreCase(s: string, a: u8, b: u8, c: u8): bool;
export fn fnmMatchAt(buf: string, pos: usize, pat: string): bool;
export fn fnmEndsWith(buf: string, len: usize, pat: string): bool;
export fn fnmAppendStr(dst: []u8, pos: usize, src: string): usize;
export fn fnmUtf8SafeLen(s: string, max_len: usize): usize;
export fn isReservedFilename(name: string): bool;
export fn filenamifyWithOptions(input: string, replacement: string, max_len: usize, out: []u8): usize;
export fn filenamify(input: string, out: []u8): usize;
