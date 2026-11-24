// Source: Absorbed from /data/dev/bun/src/sourcemap/VLQ.cp | Synced: 2026-11-24T04:33:36Z

pub const MAX_VLQ_BYTES: usize = 7;
pub const BASE64_ALPHABET: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

pub err Error {
    InvalidBase64Char,
    TruncatedInput,
    Overflow,
    BufferTooSmall,
    OutOfBounds,
}

/// 单个 Base64-VLQ 编码结果（栈上定长 7 字节，零堆分配）
pub struct VLQ {
    bytes: [u8; 7] = [0; 7],
    len: u8 = 0,

    /// 返回有效编码字节切片视图
    fn slice(): string {
        return _.bytes[0 .. _.len as usize];
    }

    /// 写入目标缓冲区并返回写入字节数
    fn copyTo(out: []u8): usize! {
        const n = _.len as usize;
        if (out.len < n) {
            return err.BufferTooSmall;
        }
        @memcpy(out[0..n], _.bytes[0..n]);
        return n;
    }

    /// 写入任意具备 writeAll 接口的 writer
    fn writeTo(writer: auto)! {
        writer.writeAll(_.bytes[0 .. _.len as usize])?;
    }
}

/// VLQ 解码结果（解码值 + 下一个读取偏移）
pub struct VLQResult {
    value: i32,
    next: usize,

    static fn init(value: i32, next: usize): VLQResult {
        return VLQResult{
            value: value,
            next: next,
        };
    }
}

fn makeBase64Lut(): [u8; 128] {
    let table: [u8; 128] = [0xFF; 128];
    let i: usize = 0;
    while (i < 64) {
        const ch = BASE64_ALPHABET[i];
        table[ch as usize] = i as u8;
        i += 1;
    }
    return table;
}

/// 慢路径通用编码（支持完整 i32 范围含 i32::MIN = -2147483648 与 i32::MAX = 2147483647）
pub fn encodeSlowPath(value: i32): VLQ {
    let len: u8 = 0;
    let bytes: [u8; 7] = [0; 7];

    let vlq: u64 = 0;
    if (value >= 0) {
        vlq = (value as u64) << 1;
    } else {
        const neg: i64 = -(value as i64);
        vlq = ((neg as u64) << 1) | 1;
    }

    let step: usize = 0;
    while (step < MAX_VLQ_BYTES) {
        let digit: u64 = vlq & 31;
        vlq = vlq >> 5;
        if (vlq != 0) {
            digit |= 32;
        }
        bytes[len as usize] = BASE64_ALPHABET[digit as usize];
        len += 1;
        if (vlq == 0) {
            return VLQ{ bytes: bytes, len: len };
        }
        step += 1;
    }

    return VLQ{ bytes: bytes, len: len };
}

fn makeVlqLookupTable(): [VLQ; 256] {
    @setEvalBranchQuota(20000);
    let entries: [VLQ; 256] = [VLQ{ bytes: [0; 7], len: 0 }; 256];
    let i: usize = 0;
    while (i < 256) {
        entries[i] = encodeSlowPath(i as i32);
        i += 1;
    }
    return entries;
}

const BASE64_LUT: [u8; 128] = makeBase64Lut();
const VLQ_LOOKUP_TABLE: [VLQ; 256] = makeVlqLookupTable();

/// 数值 0 的预计算 VLQ 编码（"A"）
pub const ZERO: VLQ = VLQ_LOOKUP_TABLE[0];

/// 高速 Base64-VLQ 编码（0..=255 命中 O(1) 编译期查表，其余走无溢出慢路径）
pub fn encode(value: i32): VLQ {
    if (value >= 0 && value <= 255) {
        return VLQ_LOOKUP_TABLE[value as usize];
    }
    return encodeSlowPath(value);
}

/// 将单个整数直接编码写入目标缓冲区，返回写入字节数
pub fn encodeInto(value: i32, out: []u8): usize! {
    const v = encode(value);
    return v.copyTo(out);
}

/// 将多个整数依次连续编码写入目标缓冲区（如 SourceMap 单段 4/5 字段），返回总写入字节数
pub fn encodeSliceInto(values: []const i32, out: []u8): usize! {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < values.len) {
        const written = encodeInto(values[i], out[pos..])?;
        pos += written;
        i += 1;
    }
    return pos;
}

/// 严格校验的 Base64-VLQ 解码器（拦截越界、非法 Base64 字符、截断输入与 i32 溢出）
pub fn decodeChecked(encoded: string, start: usize): VLQResult! {
    if (start >= encoded.len) {
        return err.OutOfBounds;
    }

    let shift: u6 = 0;
    let vlq: u64 = 0;
    let i: usize = start;
    let steps: usize = 0;

    while (i < encoded.len && steps < MAX_VLQ_BYTES) {
        const raw = encoded[i];
        if (raw >= 128) {
            return err.InvalidBase64Char;
        }
        const digit = BASE64_LUT[raw as usize];
        if (digit == 0xFF) {
            return err.InvalidBase64Char;
        }

        const payload: u64 = (digit & 31) as u64;
        vlq |= payload << shift;

        const has_more = (digit & 32) != 0;
        if (!has_more) {
            const is_neg = (vlq & 1) != 0;
            const mag: u64 = vlq >> 1;
            if (is_neg) {
                if (mag > 2147483648) {
                    return err.Overflow;
                }
                const signed_i64: i64 = -(mag as i64);
                return VLQResult{
                    value: signed_i64 as i32,
                    next: i + 1,
                };
            } else {
                if (mag > 2147483647) {
                    return err.Overflow;
                }
                return VLQResult{
                    value: mag as i32,
                    next: i + 1,
                };
            }
        }

        if (shift >= 30) {
            return err.Overflow;
        }
        shift += 5;
        i += 1;
        steps += 1;
    }

    if (steps >= MAX_VLQ_BYTES) {
        return err.Overflow;
    }
    return err.TruncatedInput;
}

/// 可选解码封装：非法字符、截断或溢出时返回 null
pub fn decode(encoded: string, start: usize): VLQResult? {
    return decodeChecked(encoded, start) catch return null;
}

/// 连续解码 SourceMap 单个映射段中的多个 VLQ 字段，返回解码字段数
pub fn decodeSegment(segment: string, out: []i32): usize! {
    let pos: usize = 0;
    let count: usize = 0;
    while (pos < segment.len) {
        if (count >= out.len) {
            return err.BufferTooSmall;
        }
        const res = decodeChecked(segment, pos)?;
        out[count] = res.value;
        count += 1;
        pos = res.next;
    }
    return count;
}
