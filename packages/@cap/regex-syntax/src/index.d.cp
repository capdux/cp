// Type definitions for CP

export class RegexSyntaxInfo {
    valid: bool,
    err_code: i32,
    err_pos: usize,
    capture_count: usize,
    has_anchors: bool,
    has_backref: bool,
    min_len: usize,
    literal_prefix_len: usize,
    fn isOk(): bool;
}

export class RsxStep {
    ok: bool,
    err_code: i32,
    err_pos: usize,
    next_pos: usize,
    val_a: usize,
    val_b: usize,
    flag: bool,
}

export fn isMetaCharacter(c: u8): bool;
export fn escapeRegex(input: string, out: []u8): usize;
export fn isWordByte(b: u8): bool;
export fn rsxIsDigit(b: u8): bool;
export fn rsxHexVal(b: u8): i32;
export fn rsxIsIdentStart(b: u8): bool;
export fn rsxIsFlagChar(b: u8): bool;
export fn rsxMakeError(code: i32, pos: usize, caps: usize, anchors: bool, backref: bool): RegexSyntaxInfo;
export fn rsxParseEscape(pattern: string, pos: usize): RsxStep;
export fn rsxParseClassItem(pattern: string, pos: usize): RsxStep;
export fn rsxParseCharClass(pattern: string, start_pos: usize): RsxStep;
export fn rsxParseBraceQuant(pattern: string, start_pos: usize): RsxStep;
export fn rsxParseGroupHeader(pattern: string, start_pos: usize): RsxStep;
export fn rsxHasTopLevelAlt(pattern: string): bool;
export fn rsxExtractPrefixCore(pattern: string, out: []u8, write_out: bool): usize;
export fn extractLiteralPrefix(pattern: string, out: []u8): usize;
export fn validateRegexSyntax(pattern: string): RegexSyntaxInfo;
