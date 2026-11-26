// Type definitions for CP

export class U64Decode {
    ok: bool,
    value: u64,
    bytes_read: usize,
    fn fail(): U64Decode;
}

export class I64Decode {
    ok: bool,
    value: i64,
    bytes_read: usize,
    fn fail(): I64Decode;
}

export fn i64ToBits(v: i64): u64;
export fn bitsToI64(u: u64): i64;
export fn uleb128Size(v: u64): usize;
export fn encodeUleb128(v: u64, out: []u8): usize?;
export fn decodeUleb128(data: string): U64Decode;
export fn zigzagEncode(v: i64): u64;
export fn zigzagDecode(u: u64): i64;
export fn encodeSleb128(v: i64, out: []u8): usize?;
export fn decodeSleb128(data: string): I64Decode;
export fn encodeZigzagVarint(v: i64, out: []u8): usize?;
export fn decodeZigzagVarint(data: string): I64Decode;
export fn encodeQuicVarint(v: u64, out: []u8): usize?;
export fn decodeQuicVarint(data: string): U64Decode;
