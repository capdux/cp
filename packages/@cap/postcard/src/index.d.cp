// Type definitions for CP

export class VarintU64Res {
    ok: bool,
    value: u64,
    consumed: usize,
}

export class VarintI64Res {
    ok: bool,
    value: i64,
    consumed: usize,
}

export fn pcrdU64Max(): u64;
export fn encodeVarintU64(val: u64, out: []u8): usize;
export fn decodeVarintU64(data: &[]u8): VarintU64Res;
export fn zigzagEncode(val: i64): u64;
export fn zigzagDecode(val: u64): i64;
export fn encodeVarintI64(val: i64, out: []u8): usize;
export fn decodeVarintI64(data: &[]u8): VarintI64Res;
export fn cobsEncode(src: &[]u8, out: []u8): usize;
export fn cobsDecode(src: &[]u8, out: []u8): i64;
