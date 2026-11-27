// Type definitions for CP

export class OptStrResult {
    ok: bool,
    has_value: bool,
    value: string,
}

export fn zigzagEncode(v: i64): u64;
export fn zigzagDecode(u: u64): i64;
export class BincodeEncoder {
    out: []u8,
    pos: usize,
    overflow: bool,
    fn init(out: []u8): BincodeEncoder;
    fn asSlice(): string;
    fn writeByte(_: auto, b: u8): bool;
    fn encodeBool(_: auto, v: bool): bool;
    fn encodeFixedU32(_: auto, v: u32): bool;
    fn encodeFixedU64(_: auto, v: u64): bool;
    fn encodeU64(_: auto, v: u64): bool;
    fn encodeI64(_: auto, v: i64): bool;
    fn encodeStr(_: auto, s: string): bool;
    fn encodeOptStr(_: auto, s: string?): bool;
}

export class BincodeDecoder {
    data: string,
    pos: usize,
    fn init(data: string): BincodeDecoder;
    fn remaining(): usize;
    fn atEnd(): bool;
    fn decodeBool(_: auto): bool?;
    fn decodeFixedU32(_: auto): u32?;
    fn decodeFixedU64(_: auto): u64?;
    fn decodeU64(_: auto): u64?;
    fn decodeI64(_: auto): i64?;
    fn decodeStr(_: auto): string?;
    fn decodeOptStr(_: auto): OptStrResult;
}

