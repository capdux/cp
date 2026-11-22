// Origin: Pure-CP Stdlib (Zero-Alloc Jackson Smile 1.0 Binary JSON Parser) | Created: 2026-11-22

/// Jackson Smile 1.0 魔数、标志位与终止符常量
pub const SMILE_HEADER_MAGIC: []const u8 = ":)\n";
pub const SMILE_HEADER_SIZE: usize = 4;
pub const SMILE_FLAG_SHARED_NAMES: u8 = 0x01;
pub const SMILE_FLAG_SHARED_VALUES: u8 = 0x02;
pub const SMILE_FLAG_RAW_BINARY: u8 = 0x04;
pub const SMILE_END_OF_STRING: u8 = 0xFC;
pub const SMILE_END_OF_CONTENT: u8 = 0xFF;

/// Smile 4 字节文件头解析结果
pub struct SmileHeader {
    version: u8,
    shared_names: bool,
    shared_values: bool,
    raw_binary: bool,
    flags: u8,
}

/// Smile 7-bit+6-bit 末字节 VInt 解码结果
pub struct SmileVInt {
    value: u64,
    next_pos: usize,
}

/// Smile 值 Token 类型枚举
pub enum SmileTokenKind: u8 {
    empty_string = 0,
    null_val = 1,
    false_val = 2,
    true_val = 3,
    int_val = 4,
    float32 = 5,
    float64 = 6,
    tiny_ascii = 7,
    small_ascii = 8,
    tiny_unicode = 9,
    small_unicode = 10,
    long_ascii = 11,
    long_unicode = 12,
    shared_str_ref = 13,
    start_array = 14,
    end_array = 15,
    start_object = 16,
    end_object = 17,
    end_of_content = 18,
}

/// Smile 对象键 Token 类型枚举
pub enum SmileKeyKind: u8 {
    empty_key = 0,
    short_ref = 1,
    long_ref = 2,
    short_ascii = 3,
    short_unicode = 4,
    long_key = 5,
    end_object = 6,
}

/// Smile 值 Token 零拷贝视图
pub struct SmileToken {
    kind: SmileTokenKind,
    int_val: i64,
    float_val: f64,
    str_val: []const u8,
    ref_index: u32,
}

/// Smile 对象键 Token 零拷贝视图
pub struct SmileKeyToken {
    kind: SmileKeyKind,
    text: []const u8,
    ref_index: u32,
}

/// 校验字节切片是否以 4 字节 Jackson Smile 魔数 `":)\n"` 开头
pub fn isSmile(data: []const u8): bool {
    if (data.len < SMILE_HEADER_SIZE) return false;
    return data[0] == 0x3A && data[1] == 0x29 && data[2] == 0x0A;
}

/// 解析 4 字节 Jackson Smile 头（版本号与 3 个特性标志位）
pub fn parseSmileHeader(data: []const u8): SmileHeader? {
    if (!isSmile(data)) return null;
    const b3 = data[3];
    return SmileHeader {
        version: (b3 >> 4) & 0x0F,
        shared_names: (b3 & SMILE_FLAG_SHARED_NAMES) != 0,
        shared_values: (b3 & SMILE_FLAG_SHARED_VALUES) != 0,
        raw_binary: (b3 & SMILE_FLAG_RAW_BINARY) != 0,
        flags: b3 & 0x0F,
    };
}

/// 解码 Smile 大端 VInt（前置字节最高位为 0 含 7 位数据；末字节最高位为 1 仅含低 6 位数据）
pub fn readSmileVInt(data: []const u8, pos: usize): SmileVInt? {
    if (pos >= data.len) return null;

    let acc: u64 = 0;
    let p = pos;
    let steps: usize = 0;
    while (p < data.len && steps < 10) {
        const b = data[p];
        p += 1;
        steps += 1;
        if ((b & 0x80) != 0) {
            if (acc > (0xFFFFFFFFFFFFFFFF >> 6)) return null;
            acc = (acc << 6) | ((b & 0x3F) as u64);
            return SmileVInt { value: acc, next_pos: p };
        }
        if (acc > (0xFFFFFFFFFFFFFFFF >> 7)) return null;
        acc = (acc << 7) | ((b & 0x7F) as u64);
    }
    return null;
}

/// ZigZag 解码：`(v >> 1) ^ -(v & 1)`，无溢出覆盖完整 `i64`
pub fn smileZigZagDecode(v: u64): i64 {
    const half = v >> 1;
    if ((v & 1) == 0) {
        return half as i64;
    }
    return -(half as i64) - 1;
}

fn makeSimpleToken(kind: SmileTokenKind): SmileToken {
    return SmileToken {
        kind: kind,
        int_val: 0,
        float_val: 0.0,
        str_val: "",
        ref_index: 0,
    };
}

/// 零堆分配 Jackson Smile 流式 Token 解码器
pub struct SmileReader {
    data: []const u8,
    pos: usize = SMILE_HEADER_SIZE,
    header: SmileHeader,
    had_error: bool = false,

    static fn init(data: []const u8): SmileReader {
        if (hdr := parseSmileHeader(data)) {
            return SmileReader {
                data: data,
                pos: SMILE_HEADER_SIZE,
                header: hdr,
                had_error: false,
            };
        }
        return SmileReader {
            data: data,
            pos: data.len,
            header: SmileHeader {
                version: 0,
                shared_names: false,
                shared_values: false,
                raw_binary: false,
                flags: 0,
            },
            had_error: true,
        };
    }

    /// 读取下一条值 Token
    fn next(*_): SmileToken? {
        if (_.had_error || _.pos >= _.data.len) return null;

        const b = _.data[_.pos];
        _.pos += 1;

        // Literals
        if (b == 0x20) return makeSimpleToken(SmileTokenKind.empty_string);
        if (b == 0x21) return makeSimpleToken(SmileTokenKind.null_val);
        if (b == 0x22) return makeSimpleToken(SmileTokenKind.false_val);
        if (b == 0x23) return makeSimpleToken(SmileTokenKind.true_val);

        // Containers & end-of-content
        if (b == 0xF8) return makeSimpleToken(SmileTokenKind.start_array);
        if (b == 0xF9) return makeSimpleToken(SmileTokenKind.end_array);
        if (b == 0xFA) return makeSimpleToken(SmileTokenKind.start_object);
        if (b == 0xFB) return makeSimpleToken(SmileTokenKind.end_object);
        if (b == SMILE_END_OF_CONTENT) return makeSimpleToken(SmileTokenKind.end_of_content);

        // Short shared string value reference (0x01..0x1F)
        if (b >= 0x01 && b <= 0x1F) {
            return SmileToken {
                kind: SmileTokenKind.shared_str_ref,
                int_val: 0,
                float_val: 0.0,
                str_val: "",
                ref_index: (b - 1) as u32,
            };
        }

        // Long shared string value reference (0xEC..0xEF, 10-bit)
        if (b >= 0xEC && b <= 0xEF) {
            if (_.pos >= _.data.len) {
                _.had_error = true;
                return null;
            }
            const lo = _.data[_.pos];
            _.pos += 1;
            const idx: u32 = (((b & 0x03) as u32) << 8) | (lo as u32);
            return SmileToken {
                kind: SmileTokenKind.shared_str_ref,
                int_val: 0,
                float_val: 0.0,
                str_val: "",
                ref_index: idx,
            };
        }

        // Inline small ZigZag integer (0xC0..0xDF => -16..15)
        if (b >= 0xC0 && b <= 0xDF) {
            const iv = smileZigZagDecode((b & 0x1F) as u64);
            return SmileToken {
                kind: SmileTokenKind.int_val,
                int_val: iv,
                float_val: 0.0,
                str_val: "",
                ref_index: 0,
            };
        }

        // VInt32 (0x24) / VInt64 (0x25)
        if (b == 0x24 || b == 0x25) {
            if (v := readSmileVInt(_.data, _.pos)) {
                _.pos = v.next_pos;
                return SmileToken {
                    kind: SmileTokenKind.int_val,
                    int_val: smileZigZagDecode(v.value),
                    float_val: 0.0,
                    str_val: "",
                    ref_index: 0,
                };
            }
            _.had_error = true;
            return null;
        }

        // 5-byte 7-bit-encoded f32 (0x28)
        if (b == 0x28) {
            if (_.data.len - _.pos < 5) {
                _.had_error = true;
                return null;
            }
            const p = _.pos;
            const bits: u32 = (((_.data[p] & 0x0F) as u32) << 28)
                | (((_.data[p + 1] & 0x7F) as u32) << 21)
                | (((_.data[p + 2] & 0x7F) as u32) << 14)
                | (((_.data[p + 3] & 0x7F) as u32) << 7)
                | ((_.data[p + 4] & 0x7F) as u32);
            _.pos = p + 5;
            const f: f32 = @bitCast(bits);
            return SmileToken {
                kind: SmileTokenKind.float32,
                int_val: 0,
                float_val: f as f64,
                str_val: "",
                ref_index: 0,
            };
        }

        // 10-byte 7-bit-encoded f64 (0x29)
        if (b == 0x29) {
            if (_.data.len - _.pos < 10) {
                _.had_error = true;
                return null;
            }
            const p = _.pos;
            let bits: u64 = (_.data[p] & 0x01) as u64;
            let i: usize = 1;
            while (i < 10) {
                bits = (bits << 7) | ((_.data[p + i] & 0x7F) as u64);
                i += 1;
            }
            _.pos = p + 10;
            const d: f64 = @bitCast(bits);
            return SmileToken {
                kind: SmileTokenKind.float64,
                int_val: 0,
                float_val: d,
                str_val: "",
                ref_index: 0,
            };
        }

        // Tiny ASCII (0x40..0x5F => 1..32B)
        if (b >= 0x40 && b <= 0x5F) {
            const len: usize = ((b - 0x40) as usize) + 1;
            if (_.data.len - _.pos < len) {
                _.had_error = true;
                return null;
            }
            const s = _.data[_.pos .. _.pos + len];
            _.pos += len;
            return SmileToken { kind: SmileTokenKind.tiny_ascii, int_val: 0, float_val: 0.0, str_val: s, ref_index: 0 };
        }

        // Small ASCII (0x60..0x7F => 33..64B)
        if (b >= 0x60 && b <= 0x7F) {
            const len: usize = ((b - 0x60) as usize) + 33;
            if (_.data.len - _.pos < len) {
                _.had_error = true;
                return null;
            }
            const s = _.data[_.pos .. _.pos + len];
            _.pos += len;
            return SmileToken { kind: SmileTokenKind.small_ascii, int_val: 0, float_val: 0.0, str_val: s, ref_index: 0 };
        }

        // Tiny Unicode (0x80..0x9F => 2..33B)
        if (b >= 0x80 && b <= 0x9F) {
            const len: usize = ((b - 0x80) as usize) + 2;
            if (_.data.len - _.pos < len) {
                _.had_error = true;
                return null;
            }
            const s = _.data[_.pos .. _.pos + len];
            _.pos += len;
            return SmileToken { kind: SmileTokenKind.tiny_unicode, int_val: 0, float_val: 0.0, str_val: s, ref_index: 0 };
        }

        // Small Unicode (0xA0..0xBF => 34..65B)
        if (b >= 0xA0 && b <= 0xBF) {
            const len: usize = ((b - 0xA0) as usize) + 34;
            if (_.data.len - _.pos < len) {
                _.had_error = true;
                return null;
            }
            const s = _.data[_.pos .. _.pos + len];
            _.pos += len;
            return SmileToken { kind: SmileTokenKind.small_unicode, int_val: 0, float_val: 0.0, str_val: s, ref_index: 0 };
        }

        // Long ASCII (0xE0) / Long Unicode (0xE4), terminated by 0xFC
        if (b == 0xE0 || b == 0xE4) {
            const start = _.pos;
            let end = start;
            let found = false;
            while (end < _.data.len) {
                if (_.data[end] == SMILE_END_OF_STRING) {
                    found = true;
                    break;
                }
                end += 1;
            }
            if (!found) {
                _.had_error = true;
                return null;
            }
            _.pos = end + 1;
            const kind = if (b == 0xE0) SmileTokenKind.long_ascii else SmileTokenKind.long_unicode;
            return SmileToken { kind: kind, int_val: 0, float_val: 0.0, str_val: _.data[start..end], ref_index: 0 };
        }

        _.had_error = true;
        return null;
    }

    /// 对象键模式读取器（支持空键、短/长键引用、短 ASCII/Unicode 键、0xFC 终止长键与 0xFB 对象结束）
    fn readKey(*_): SmileKeyToken? {
        if (_.had_error || _.pos >= _.data.len) return null;

        const b = _.data[_.pos];
        _.pos += 1;

        if (b == 0x20) {
            return SmileKeyToken { kind: SmileKeyKind.empty_key, text: "", ref_index: 0 };
        }
        if (b == 0xFB) {
            return SmileKeyToken { kind: SmileKeyKind.end_object, text: "", ref_index: 0 };
        }
        if (b >= 0x30 && b <= 0x33) {
            if (_.pos >= _.data.len) {
                _.had_error = true;
                return null;
            }
            const lo = _.data[_.pos];
            _.pos += 1;
            const idx: u32 = (((b & 0x03) as u32) << 8) | (lo as u32);
            return SmileKeyToken { kind: SmileKeyKind.long_ref, text: "", ref_index: idx };
        }
        if (b == 0x34) {
            const start = _.pos;
            let end = start;
            let found = false;
            while (end < _.data.len) {
                if (_.data[end] == SMILE_END_OF_STRING) {
                    found = true;
                    break;
                }
                end += 1;
            }
            if (!found) {
                _.had_error = true;
                return null;
            }
            _.pos = end + 1;
            return SmileKeyToken { kind: SmileKeyKind.long_key, text: _.data[start..end], ref_index: 0 };
        }
        if (b >= 0x40 && b <= 0x7F) {
            return SmileKeyToken { kind: SmileKeyKind.short_ref, text: "", ref_index: (b - 0x40) as u32 };
        }
        if (b >= 0x80 && b <= 0xBF) {
            const len: usize = ((b - 0x80) as usize) + 1;
            if (_.data.len - _.pos < len) {
                _.had_error = true;
                return null;
            }
            const s = _.data[_.pos .. _.pos + len];
            _.pos += len;
            return SmileKeyToken { kind: SmileKeyKind.short_ascii, text: s, ref_index: 0 };
        }
        if (b >= 0xC0 && b <= 0xF7) {
            const len: usize = ((b - 0xC0) as usize) + 2;
            if (_.data.len - _.pos < len) {
                _.had_error = true;
                return null;
            }
            const s = _.data[_.pos .. _.pos + len];
            _.pos += len;
            return SmileKeyToken { kind: SmileKeyKind.short_unicode, text: s, ref_index: 0 };
        }

        _.had_error = true;
        return null;
    }
}
