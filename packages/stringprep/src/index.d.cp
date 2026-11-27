// Type definitions for CP

export class PrepResult {
    len: usize,
    ok: bool,
}

export class DecCp {
    cp: u32,
    width: usize,
    valid: bool,
}

export fn decodeUtf8(s: string, i: usize): DecCp;
export fn encodeUtf8(cp: u32, out: []u8, pos: usize): i64;
export fn isMappedToNothing(cp: u32): bool;
export fn isNonAsciiSpace(cp: u32): bool;
export fn isSaslProhibited(cp: u32): bool;
export fn saslprep(input: string, out: []u8): PrepResult;
export fn nameprepAscii(input: string, out: []u8): PrepResult;
