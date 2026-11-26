// Type definitions for CP

export class RespFrame {
    kind: u8,
    data_start: usize,
    data_len: usize,
    int_val: i64,
    array_len: i64,
    consumed: usize,
}

export fn makeFrame(kind: u8): RespFrame;
export class IntResult {
    value: i64,
    ok: bool,
}

export fn parseI64(s: &[]u8): IntResult;
export fn findCrlf(buf: &[]u8, start: usize): i64;
export fn parseFrame(buf: &[]u8): RespFrame;
export fn appendByte(dst: []u8, pos: usize, b: u8): i64;
export fn appendSlice(dst: []u8, pos: usize, s: &[]u8): i64;
export fn appendI64(dst: []u8, pos: usize, v: i64): i64;
export fn encodeSimpleString(s: &[]u8, dst: []u8): i64;
export fn encodeInteger(v: i64, dst: []u8): i64;
export fn encodeNull(dst: []u8): i64;
export fn encodeBulkString(s: &[]u8, dst: []u8): i64;
export fn encodeCommand(args: &[]&[]u8, dst: []u8): i64;
