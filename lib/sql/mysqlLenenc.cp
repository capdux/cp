// Source: Absorbed from /data/dev/bun/src/sql/mysql/protocol/EncodeInt.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation MySQL wire-protocol length-encoded integer (`lenenc-int`)
// and length-encoded string (`lenenc-str`) encoder/decoder (1/3/4/9-byte LE format,
// with 0xfb NULL & 0xff ERR marker awareness and truncation/overflow guards).

/// MySQL 协议 length-encoded NULL 标记字节（251）
pub const MYSQL_LENENC_NULL_MARKER: u8 = 0xfb;
/// MySQL 协议 2 字节小端整数前导标记（252）
pub const MYSQL_LENENC_U16_MARKER: u8 = 0xfc;
/// MySQL 协议 3 字节小端整数前导标记（253）
pub const MYSQL_LENENC_U24_MARKER: u8 = 0xfd;
/// MySQL 协议 8 字节小端整数前导标记（254）
pub const MYSQL_LENENC_U64_MARKER: u8 = 0xfe;
/// MySQL 协议 ERR 包前导标记（255）
pub const MYSQL_LENENC_ERR_MARKER: u8 = 0xff;

/// MySQL Length-Encoded 编解码错误集
pub err MySqlLenEncError {
    BufferTooSmall,
    TruncatedInput,
    NullMarker,
    ErrMarker,
    LengthOverflow,
}

/// 固定 9 字节栈上编码结果（零堆分配）
pub struct EncodedLengthInt {
    bytes: [9]u8 = [0; 9],
    len: usize = 0,

    fn slice(): []const u8 {
        return _.bytes[0.._.len];
    }
}

/// 变长整数解码结果
pub struct DecodeLengthIntResult {
    value: u64,
    bytes_read: usize,
}

/// 长度前缀字符串零拷贝解码结果
pub struct DecodeLengthStringResult {
    data: []const u8,
    bytes_read: usize,
}

/// 计算 `value` 编码为 MySQL `lenenc-int` 所需的精确字节数（1 / 3 / 4 / 9）
pub fn encodedLengthIntSize(value: u64): usize {
    if (value < 0xfb) {
        return 1;
    }
    if (value <= 0xffff) {
        return 3;
    }
    if (value <= 0xffffff) {
        return 4;
    }
    return 9;
}

/// 将 64 位无符号整数编码为 MySQL 协议 `lenenc-int`（返回 9 字节固定栈对象）
pub fn encodeLengthInt(value: u64): EncodedLengthInt {
    let out = EncodedLengthInt{
        bytes: [0; 9],
        len: 0,
    };
    if (value < 0xfb) {
        out.len = 1;
        out.bytes[0] = @intCast(value);
    } else if (value <= 0xffff) {
        out.len = 3;
        out.bytes[0] = MYSQL_LENENC_U16_MARKER;
        out.bytes[1] = @intCast(value & 0xff);
        out.bytes[2] = @intCast((value >> 8) & 0xff);
    } else if (value <= 0xffffff) {
        out.len = 4;
        out.bytes[0] = MYSQL_LENENC_U24_MARKER;
        out.bytes[1] = @intCast(value & 0xff);
        out.bytes[2] = @intCast((value >> 8) & 0xff);
        out.bytes[3] = @intCast((value >> 16) & 0xff);
    } else {
        out.len = 9;
        out.bytes[0] = MYSQL_LENENC_U64_MARKER;
        out.bytes[1] = @intCast(value & 0xff);
        out.bytes[2] = @intCast((value >> 8) & 0xff);
        out.bytes[3] = @intCast((value >> 16) & 0xff);
        out.bytes[4] = @intCast((value >> 24) & 0xff);
        out.bytes[5] = @intCast((value >> 32) & 0xff);
        out.bytes[6] = @intCast((value >> 40) & 0xff);
        out.bytes[7] = @intCast((value >> 48) & 0xff);
        out.bytes[8] = @intCast((value >> 56) & 0xff);
    }
    return out;
}

/// 直接将 `lenenc-int` 写入调用方缓冲区，返回写入字节数
pub fn encodeLengthIntInto(value: u64, out: []u8): usize! {
    const enc = encodeLengthInt(value);
    if (out.len < enc.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < enc.len) {
        out[i] = enc.bytes[i];
        i += 1;
    }
    return enc.len;
}

/// 兼容 Bun `decodeLengthInt` 语义：解码 1/3/4/9 字节 `lenenc-int`，输入不足返回 `null`
pub fn decodeLengthInt(bytes: []const u8): DecodeLengthIntResult? {
    if (bytes.len == 0) {
        return null;
    }
    const first = bytes[0];
    if (first == MYSQL_LENENC_U16_MARKER) {
        if (bytes.len < 3) {
            return null;
        }
        const v: u64 = @as(u64, bytes[1]) | (@as(u64, bytes[2]) << 8);
        return DecodeLengthIntResult{ value: v, bytes_read: 3 };
    }
    if (first == MYSQL_LENENC_U24_MARKER) {
        if (bytes.len < 4) {
            return null;
        }
        const v: u64 = @as(u64, bytes[1]) |
            (@as(u64, bytes[2]) << 8) |
            (@as(u64, bytes[3]) << 16);
        return DecodeLengthIntResult{ value: v, bytes_read: 4 };
    }
    if (first == MYSQL_LENENC_U64_MARKER) {
        if (bytes.len < 9) {
            return null;
        }
        const v: u64 = @as(u64, bytes[1]) |
            (@as(u64, bytes[2]) << 8) |
            (@as(u64, bytes[3]) << 16) |
            (@as(u64, bytes[4]) << 24) |
            (@as(u64, bytes[5]) << 32) |
            (@as(u64, bytes[6]) << 40) |
            (@as(u64, bytes[7]) << 48) |
            (@as(u64, bytes[8]) << 56);
        return DecodeLengthIntResult{ value: v, bytes_read: 9 };
    }
    return DecodeLengthIntResult{ value: @as(u64, first), bytes_read: 1 };
}

/// 严格模式解码 `lenenc-int`：显式区分 `TruncatedInput`、`NullMarker (0xfb)` 与 `ErrMarker (0xff)`
pub fn decodeLengthIntStrict(bytes: []const u8): DecodeLengthIntResult! {
    if (bytes.len == 0) {
        return err.TruncatedInput;
    }
    const first = bytes[0];
    if (first == MYSQL_LENENC_NULL_MARKER) {
        return err.NullMarker;
    }
    if (first == MYSQL_LENENC_ERR_MARKER) {
        return err.ErrMarker;
    }
    if (res := decodeLengthInt(bytes)) {
        return res;
    }
    return err.TruncatedInput;
}

/// 将字节串编码为 MySQL 协议 `lenenc-str`（`lenenc-int(data.len) || data`），返回写入总字节数
pub fn encodeLengthString(data: []const u8, out: []u8): usize! {
    const hdr_len = encodedLengthIntSize(@as(u64, data.len));
    if (out.len < hdr_len || out.len - hdr_len < data.len) {
        return err.BufferTooSmall;
    }
    const written_hdr = encodeLengthIntInto(@as(u64, data.len), out)?;
    let i: usize = 0;
    while (i < data.len) {
        out[written_hdr + i] = data[i];
        i += 1;
    }
    return written_hdr + data.len;
}

/// 零拷贝解码 MySQL 协议 `lenenc-str`，返回指向原始缓冲区的切片与总消费字节数
pub fn decodeLengthString(bytes: []const u8): DecodeLengthStringResult! {
    const hdr = decodeLengthIntStrict(bytes)?;
    const rem = bytes.len - hdr.bytes_read;
    if (hdr.value > @as(u64, rem)) {
        if (hdr.value > @as(u64, 0x7fffffff)) {
            return err.LengthOverflow;
        }
        return err.TruncatedInput;
    }
    const str_len: usize = @intCast(hdr.value);
    const end = hdr.bytes_read + str_len;
    return DecodeLengthStringResult{
        data: bytes[hdr.bytes_read..end],
        bytes_read: end,
    };
}
