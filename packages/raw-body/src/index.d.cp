// Type definitions for CP

export fn rwbIsSpace(b: u8): bool;
export fn rwbIsDigit(b: u8): bool;
export fn rwbToLower(b: u8): u8;
export fn rwbIsCont(b: u8): bool;
export fn rwbUnitEq(unit: string, target: string): bool;
export fn rwbUnitMultiplier(unit: string): u64;
export fn isValidUtf8(data: &[]u8): bool;
export fn parseByteLimit(spec: string): usize?;
export class RawBodyOptions {
    limit: usize,
    expected_len: usize,
    has_expected_len: bool,
    validate_utf8: bool,
    fn defaultOpts(): RawBodyOptions;
    fn withLimit(limit: usize): RawBodyOptions;
}

export class RawBodyResult {
    status: u16,
    bytes_written: usize,
    fn isOk(): bool;
}

export class RawBodyReader {
    opts: RawBodyOptions,
    received: usize,
    failed_status: u16,
    finished: bool,
    fn new(opts: RawBodyOptions): RawBodyReader;
    fn checkDeclaredLength(): u16;
    fn pushChunk(_: auto, chunk: &[]u8, out: []u8): u16;
    fn abort(_: auto): u16;
    fn finish(_: auto, out: &[]u8): RawBodyResult;
    fn receivedBytes(): usize;
}

