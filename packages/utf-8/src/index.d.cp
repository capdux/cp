// Type definitions for CP

export enum Utf8DecodeStatus {
    Ok,
    InvalidSequence,
    Incomplete
}

export class Utf8ChunkResult {
    status: Utf8DecodeStatus,
    valid_prefix: string,
    consumed: usize,
    error_len: usize,
}

export fn u8dExpectedLen(b0: u8): usize;
export fn u8dIsValidSecond(b0: u8, b1: u8): bool;
export fn u8dIsValidCont(b0: u8, idx: usize, b: u8): bool;
export fn u8dWriteSlice(out: []u8, pos: usize, src: &[]u8): usize;
export fn u8dWriteReplacement(out: []u8, pos: usize): usize;
export fn decodeUtf8Chunk(input: &[]u8): Utf8ChunkResult;
export fn isValidUtf8(input: &[]u8): bool;
export class LossyUtf8Decoder {
    pending: [4]u8,
    pending_len: usize,
    fn new(): LossyUtf8Decoder;
    fn hasPending(): bool;
    fn pendingLen(): usize;
    fn reset(_: auto);
    fn push(_: auto, chunk: &[]u8, out: []u8): usize;
    fn finish(_: auto, out: []u8): usize;
}

