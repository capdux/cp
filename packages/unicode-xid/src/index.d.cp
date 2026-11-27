// Type definitions for CP

export class UxidRune {
    cp: u32,
    len: usize,
    valid: bool,
}

export fn uxidDecodeUtf8Strict(s: string, offset: usize): UxidRune;
export fn uxidIsExtraStart(cp: u32): bool;
export fn uxidBsearchStart(cp: u32): bool;
export fn uxidBsearchCont(cp: u32): bool;
export fn uxidIsAsciiAlpha(b: u8): bool;
export fn uxidIsAsciiDigit(b: u8): bool;
export fn isXidStart(cp: u32): bool;
export fn isXidContinue(cp: u32): bool;
export fn isIdentStart(cp: u32): bool;
export fn isAsciiIdent(s: string): bool;
export fn isValidIdent(s: string): bool;
export fn scanIdentLen(s: string, start: usize): usize;
