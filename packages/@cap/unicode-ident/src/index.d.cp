// Type definitions for CP

export class Utf8Codepoint {
    cp: u32,
    len: usize,
    ok: bool,
}

export fn uidInRange(cp: u32, lo: u32, hi: u32): bool;
export fn decodeUtf8At(s: string, offset: usize): Utf8Codepoint;
export fn uidIsXidStartNonAscii(cp: u32): bool;
export fn uidIsXidContinueExtra(cp: u32): bool;
export fn isXidStart(cp: u32): bool;
export fn isXidContinue(cp: u32): bool;
export fn isIdentStart(cp: u32): bool;
export fn scanIdentLen(s: string): usize;
export fn isValidIdent(s: string): bool;
