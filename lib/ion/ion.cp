// Origin: Pure-CP Stdlib (Zero-Alloc Amazon Ion 1.0 Binary Parser) | Created: 2026-11-22

/// Amazon Ion 1.0 4 字节版本魔数（IVM: 0xE0 0x01 0x00 0xEA）
pub const ION_IVM_1_0: []const u8 = "\xE0\x01\x00\xEA";

/// Amazon Ion 1.0 类型描述符高 4 位枚举（Type Code T = 0..15）
pub enum IonType: u8 {
    null_nop = 0,
    bool_val = 1,
    pos_int = 2,
    neg_int = 3,
    float_val = 4,
    decimal = 5,
    timestamp = 6,
    symbol = 7,
    string = 8,
    clob = 9,
    blob = 10,
    list = 11,
    sexp = 12,
    struct_val = 13,
    annotation = 14,
    reserved = 15,
}

/// Ion VarUInt 解码结果（大端 7-bit 分组，bit 7 = 1 表示末字节）
pub struct IonVarUInt {
    value: u64,
    next_pos: usize,
}

/// Ion VarInt 解码结果（首字节 bit 6 为符号位，bit 7 = 1 表示末字节）
pub struct IonVarInt {
    value: i64,
    is_negative: bool,
    next_pos: usize,
}

/// 零拷贝 Ion 二进制值视图
pub struct IonValue {
    ion_type: IonType,
    is_null: bool,
    l_nibble: u8,
    payload: []const u8,
    total_bytes: usize,
}

/// Ion Struct 单条字段视图（字段符号 ID + 值）
pub struct IonStructField {
    field_symbol_id: u64,
    value: IonValue,
}

/// 零堆分配 Ion 顶层值流式读取器（初始化时自动跳过首部 4B IVM 1.0，迭代时自动跳过 NOP Pad）
pub struct IonReader {
    data: []const u8,
    pos: usize = 0,

    static fn init(data: []const u8): IonReader {
        const start: usize = if (isIonBinary(data)) 4 else 0;
        return IonReader {
            data: data,
            pos: start,
        };
    }

    /// 解析并返回下一条有效 Ion 值（自动跳过 NOP Pad）；到达流尾或遇到畸形编码时返回 `null`
    fn next(*_): IonValue? {
        while (_.pos < _.data.len) {
            const v_opt = parseRawIonValue(_.data, _.pos);
            if (v_opt == null) {
                _.pos = _.data.len;
                return null;
            }
            const v = v_opt ?? return null;
            _.pos += v.total_bytes;
            // 自动跳过 NOP Pad（T == 0 && !is_null）
            if (v.ion_type == IonType.null_nop && !v.is_null) {
                continue;
            }
            return v;
        }
        return null;
    }
}

/// 零堆分配 Ion List / SExp 子元素迭代器
pub struct IonListIter {
    data: []const u8,
    pos: usize = 0,

    static fn init(list_payload: []const u8): IonListIter {
        return IonListIter {
            data: list_payload,
            pos: 0,
        };
    }

    fn next(*_): IonValue? {
        while (_.pos < _.data.len) {
            const v_opt = parseRawIonValue(_.data, _.pos);
            if (v_opt == null) {
                _.pos = _.data.len;
                return null;
            }
            const v = v_opt ?? return null;
            _.pos += v.total_bytes;
            if (v.ion_type == IonType.null_nop && !v.is_null) {
                continue;
            }
            return v;
        }
        return null;
    }
}

/// 零堆分配 Ion Struct 字段迭代器（交替读取 `VarUInt field_symbol_id` 与 `IonValue`）
pub struct IonStructIter {
    data: []const u8,
    pos: usize = 0,

    static fn init(struct_payload: []const u8): IonStructIter {
        return IonStructIter {
            data: struct_payload,
            pos: 0,
        };
    }

    fn next(*_): IonStructField? {
        while (_.pos < _.data.len) {
            const sid_opt = readIonVarUInt(_.data, _.pos);
            if (sid_opt == null) {
                _.pos = _.data.len;
                return null;
            }
            const sid = sid_opt ?? return null;

            const val_opt = parseRawIonValue(_.data, sid.next_pos);
            if (val_opt == null) {
                _.pos = _.data.len;
                return null;
            }
            const val = val_opt ?? return null;
            _.pos = sid.next_pos + val.total_bytes;

            // Struct 中允许以任意 field_sid + NOP Pad 填充
            if (val.ion_type == IonType.null_nop && !val.is_null) {
                continue;
            }
            return IonStructField {
                field_symbol_id: sid.value,
                value: val,
            };
        }
        return null;
    }
}

/// 检测缓冲区是否以 Amazon Ion 1.0 二进制魔数 `0xE0 0x01 0x00 0xEA` 开头
pub fn isIonBinary(data: []const u8): bool {
    if (data.len < 4) return false;
    return data[0] == 0xE0
        && data[1] == 0x01
        && data[2] == 0x00
        && data[3] == 0xEA;
}

/// 解码 Ion 大端 7-bit VarUInt（最高位 `0x80` 标记末字节；防 64-bit 溢出与截断）
pub fn readIonVarUInt(data: []const u8, pos: usize): IonVarUInt? {
    let i: usize = pos;
    let acc: u64 = 0;
    while (i < data.len) {
        if ((acc >> 57) != 0) return null;
        const b = data[i];
        acc = (acc << 7) | ((b & 0x7F) as u64);
        i += 1;
        if ((b & 0x80) != 0) {
            return IonVarUInt {
                value: acc,
                next_pos: i,
            };
        }
    }
    return null;
}

/// 解码 Ion 大端 VarInt（首字节 bit 6 为符号位，最高位 `0x80` 标记末字节）
pub fn readIonVarInt(data: []const u8, pos: usize): IonVarInt? {
    if (pos >= data.len) return null;

    const b0 = data[pos];
    const is_neg: bool = (b0 & 0x40) != 0;
    let mag: u64 = (b0 & 0x3F) as u64;
    let i: usize = pos + 1;

    if ((b0 & 0x80) == 0) {
        let done: bool = false;
        while (i < data.len) {
            if ((mag >> 57) != 0) return null;
            const b = data[i];
            mag = (mag << 7) | ((b & 0x7F) as u64);
            i += 1;
            if ((b & 0x80) != 0) {
                done = true;
                break;
            }
        }
        if (!done) return null;
    }

    if (is_neg) {
        if (mag > 9223372036854775808) return null;
        if (mag == 9223372036854775808) {
            return IonVarInt {
                value: -9223372036854775807 - 1,
                is_negative: true,
                next_pos: i,
            };
        }
        return IonVarInt {
            value: -(mag as i64),
            is_negative: true,
            next_pos: i,
        };
    }

    if (mag > 9223372036854775807) return null;
    return IonVarInt {
        value: mag as i64,
        is_negative: false,
        next_pos: i,
    };
}

fn parseRawIonValue(data: []const u8, pos: usize): IonValue? {
    if (pos >= data.len) return null;

    const td = data[pos];
    const t_code: u8 = td >> 4;
    const l_nib: u8 = td & 0x0F;
    if (t_code == 15) return null; // reserved
    const ion_type: IonType = t_code as IonType;

    // Typed null (L == 15)
    if (l_nib == 15) {
        return IonValue {
            ion_type: ion_type,
            is_null: true,
            l_nibble: 15,
            payload: data[pos + 1 .. pos + 1],
            total_bytes: 1,
        };
    }

    // Inline boolean (T == 1, L == 0 false / L == 1 true)
    if (ion_type == IonType.bool_val) {
        if (l_nib > 1) return null;
        return IonValue {
            ion_type: IonType.bool_val,
            is_null: false,
            l_nibble: l_nib,
            payload: data[pos + 1 .. pos + 1],
            total_bytes: 1,
        };
    }

    let len: usize = 0;
    let payload_start: usize = pos + 1;

    // 扩展长度：L == 14 或有序 struct 的 L == 1
    if (l_nib == 14 || (ion_type == IonType.struct_val && l_nib == 1)) {
        const len_res = readIonVarUInt(data, pos + 1) ?? return null;
        len = len_res.value as usize;
        payload_start = len_res.next_pos;
    } else {
        len = l_nib as usize;
    }

    if (len > data.len - payload_start) return null;
    const payload_end: usize = payload_start + len;

    return IonValue {
        ion_type: ion_type,
        is_null: false,
        l_nibble: l_nib,
        payload: data[payload_start..payload_end],
        total_bytes: payload_end - pos,
    };
}

fn readBeUInt(bytes: []const u8): u64? {
    if (bytes.len > 8) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < bytes.len) {
        acc = (acc << 8) | (bytes[i] as u64);
        i += 1;
    }
    return acc;
}

/// 从 `IonValue` 提取布尔值
pub fn ionAsBool(v: IonValue): bool? {
    if (v.is_null || v.ion_type != IonType.bool_val) return null;
    return v.l_nibble == 1;
}

/// 从 `IonValue` 提取无符号 64 位整数（仅限 `pos_int`）
pub fn ionAsUInt(v: IonValue): u64? {
    if (v.is_null || v.ion_type != IonType.pos_int) return null;
    return readBeUInt(v.payload);
}

/// 从 `IonValue` 提取有符号 64 位整数（支持 `pos_int` 与 `neg_int`）
pub fn ionAsInt(v: IonValue): i64? {
    if (v.is_null) return null;
    if (v.ion_type == IonType.pos_int) {
        const u = readBeUInt(v.payload) ?? return null;
        if (u > 9223372036854775807) return null;
        return u as i64;
    }
    if (v.ion_type == IonType.neg_int) {
        if (v.payload.len == 0) return null; // Ion 禁止空负载 neg_int
        const u = readBeUInt(v.payload) ?? return null;
        if (u > 9223372036854775808) return null;
        if (u == 9223372036854775808) return -9223372036854775807 - 1;
        return -(u as i64);
    }
    return null;
}

/// 从 `IonValue` 提取 IEEE 754 浮点数（0B=0.0、4B BE f32、8B BE f64）
pub fn ionAsFloat(v: IonValue): f64? {
    if (v.is_null || v.ion_type != IonType.float_val) return null;
    if (v.payload.len == 0) return 0.0;
    if (v.payload.len == 4) {
        const u = readBeUInt(v.payload) ?? return null;
        const bits: u32 = u as u32;
        const f: f32 = @as(f32, @bitCast(bits));
        return f as f64;
    }
    if (v.payload.len == 8) {
        const bits = readBeUInt(v.payload) ?? return null;
        return @as(f64, @bitCast(bits));
    }
    return null;
}

/// 从 `IonValue` 提取符号 ID（仅限 `symbol`）
pub fn ionAsSymbolId(v: IonValue): u64? {
    if (v.is_null || v.ion_type != IonType.symbol) return null;
    return readBeUInt(v.payload);
}

/// 从 `IonValue` 零拷贝提取 UTF-8 字符串切片（仅限 `string`）
pub fn ionAsString(v: IonValue): ([]const u8)? {
    if (v.is_null || v.ion_type != IonType.string) return null;
    return v.payload;
}

/// 从 `IonValue` 零拷贝提取二进制切片（支持 `blob` 与 `clob`）
pub fn ionAsBlob(v: IonValue): ([]const u8)? {
    if (v.is_null) return null;
    if (v.ion_type != IonType.blob && v.ion_type != IonType.clob) return null;
    return v.payload;
}
