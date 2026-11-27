// Type definitions for CP

import { isSpace, isDigit, trim, eqIgnoreCase } from "cap:ascii";
export fn rwbIsCont(b: u8): bool;
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

