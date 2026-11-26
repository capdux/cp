// Type definitions for CP

export class NetstringFrame {
    status: i32,
    payload_start: usize,
    payload_len: usize,
    consumed: usize,
}

export fn encodedLen(payload_len: usize): usize;
export fn encode(payload: &[]u8, dst: []u8): i64;
export fn decode(buf: &[]u8): NetstringFrame;
