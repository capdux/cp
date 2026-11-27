// Type definitions for CP

export class NetstringFrame {
    status: i32,
    payload_start: usize,
    payload_len: usize,
    consumed: usize,
    fn isOk(): bool;
    fn isIncomplete(): bool;
    fn isInvalid(): bool;
    fn payload(buf: &[]u8): &[]u8;
}

export fn encodedLen(payload_len: usize): usize;
export fn encode(payload: &[]u8, dst: []u8): i64;
export fn encodeString(payload: string, dst: []u8): i64;
export fn decode(buf: &[]u8): NetstringFrame;
export fn decodeNext(buf: &[]u8, offset: usize): NetstringFrame;
export fn countFrames(buf: &[]u8): i64;
export fn isValidNetstring(buf: &[]u8): bool;
export class NetstringDecoder {
    offset: usize,
    fn new(): NetstringDecoder;
    fn next(_: auto, buf: &[]u8): NetstringFrame;
    fn position(): usize;
    fn reset(_: auto);
}

