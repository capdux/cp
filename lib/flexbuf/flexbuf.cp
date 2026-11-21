// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Google FlatBuffers FlexBuffers Parser)

/// FlexBuffers 值类型枚举（打包类型字节高 6 位 `packed >> 2`）
pub enum FlexType: u8 {
    null_val = 0,
    int = 1,
    uint = 2,
    float = 3,
    key = 4,
    string = 5,
    indirect_int = 6,
    indirect_uint = 7,
    indirect_float = 8,
    map = 9,
    vector = 10,
    vector_int = 11,
    vector_uint = 12,
    vector_float = 13,
    vector_key = 14,
    vector_string_deprecated = 15,
    vector_int2 = 16,
    vector_uint2 = 17,
    vector_float2 = 18,
    vector_int3 = 19,
    vector_uint3 = 20,
    vector_float3 = 21,
    vector_int4 = 22,
    vector_uint4 = 23,
    vector_float4 = 24,
    blob = 25,
    bool_val = 26,
    vector_bool = 36,
}

/// FlexBuffers 位宽枚举（打包类型字节低 2 位 `packed & 0x03`，对应 1/2/4/8 字节）
pub enum FlexBitWidth: u8 {
    w8 = 0,
    w16 = 1,
    w32 = 2,
    w64 = 3,
}

/// 解包后的 `(FlexType, FlexBitWidth)` 结果
pub struct UnpackedFlexType {
    flex_type: FlexType,
    bit_width: FlexBitWidth,
}

/// 将 `FlexBitWidth` 转换为实际字节数（1 / 2 / 4 / 8）
pub fn flexBitWidthBytes(w: FlexBitWidth): usize {
    if (w == FlexBitWidth.w8) return 1;
    if (w == FlexBitWidth.w16) return 2;
    if (w == FlexBitWidth.w32) return 4;
    return 8;
}

fn isValidByteWidth(bw: usize): bool {
    return bw == 1 || bw == 2 || bw == 4 || bw == 8;
}

fn rawToFlexType(raw: u8): FlexType? {
    if (raw <= 26) return raw as FlexType;
    if (raw == 36) return FlexType.vector_bool;
    return null;
}

/// 解包 FlexBuffers 1 字节打包类型（高 6 位为 `FlexType`，低 2 位为 `FlexBitWidth`）
pub fn unpackFlexType(packed_byte: u8): UnpackedFlexType? {
    const raw_type: u8 = packed_byte >> 2;
    const ft = rawToFlexType(raw_type) ?? return null;
    const bw: FlexBitWidth = (packed_byte & 0x03) as FlexBitWidth;
    return UnpackedFlexType {
        flex_type: ft,
        bit_width: bw,
    };
}

/// 打包 `(FlexType, FlexBitWidth)` 为 1 字节类型标记（供构造与校验使用）
pub fn packFlexType(ft: FlexType, bw: FlexBitWidth): u8 {
    return ((ft as u8) << 2) | (bw as u8);
}

fn readUintWidth(data: []const u8, off: usize, width: usize): u64? {
    if (!isValidByteWidth(width)) return null;
    if (off > data.len || width > data.len - off) return null;

    let acc: u64 = 0;
    let i: usize = 0;
    while (i < width) {
        const shift: u6 = (i * 8) as u6;
        acc |= (data[off + i] as u64) << shift;
        i += 1;
    }
    return acc;
}

fn readIntWidth(data: []const u8, off: usize, width: usize): i64? {
    const u = readUintWidth(data, off, width) ?? return null;
    if (width == 1) {
        const v: i8 = (u & 0xFF) as i8;
        return v as i64;
    }
    if (width == 2) {
        const v: i16 = (u & 0xFFFF) as i16;
        return v as i64;
    }
    if (width == 4) {
        const v: i32 = (u & 0xFFFFFFFF) as i32;
        return v as i64;
    }
    return u as i64;
}

fn readFloatWidth(data: []const u8, off: usize, width: usize): f64? {
    if (width == 4) {
        const raw_u64 = readUintWidth(data, off, 4) ?? return null;
        const raw_u32: u32 = raw_u64 as u32;
        const f: f32 = @as(f32, @bitCast(raw_u32));
        return f as f64;
    }
    if (width == 8) {
        const raw_u64 = readUintWidth(data, off, 8) ?? return null;
        return @as(f64, @bitCast(raw_u64));
    }
    return null;
}

fn compareBytes(a: []const u8, b: []const u8): i32 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) return -1;
        if (a[i] > b[i]) return 1;
        i += 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

fn fixedVectorLen(ft: FlexType): usize? {
    if (ft == FlexType.vector_int2 || ft == FlexType.vector_uint2 || ft == FlexType.vector_float2) return 2;
    if (ft == FlexType.vector_int3 || ft == FlexType.vector_uint3 || ft == FlexType.vector_float3) return 3;
    if (ft == FlexType.vector_int4 || ft == FlexType.vector_uint4 || ft == FlexType.vector_float4) return 4;
    return null;
}

fn typedVectorElemType(ft: FlexType): FlexType? {
    if (ft == FlexType.vector_int || ft == FlexType.vector_int2 || ft == FlexType.vector_int3 || ft == FlexType.vector_int4) {
        return FlexType.int;
    }
    if (ft == FlexType.vector_uint || ft == FlexType.vector_uint2 || ft == FlexType.vector_uint3 || ft == FlexType.vector_uint4) {
        return FlexType.uint;
    }
    if (ft == FlexType.vector_float || ft == FlexType.vector_float2 || ft == FlexType.vector_float3 || ft == FlexType.vector_float4) {
        return FlexType.float;
    }
    if (ft == FlexType.vector_key) return FlexType.key;
    if (ft == FlexType.vector_bool) return FlexType.bool_val;
    if (ft == FlexType.vector_string_deprecated) return FlexType.string;
    return null;
}

/// 零拷贝 FlexBuffers 值引用（支持标量、间接引用、字符串/Blob/Key、多态/定型向量与有序 Map）
pub struct FlexRef {
    data: []const u8,
    offset: usize,
    parent_width: usize,
    byte_width: usize,
    flex_type: FlexType,

    /// 计算当前槽位向后相对偏移指向的目标地址
    fn indirectTarget(): usize? {
        const rel = readUintWidth(_.data, _.offset, _.parent_width) ?? return null;
        if (rel > (_.offset as u64)) return null;
        return _.offset - (rel as usize);
    }

    /// 是否为 `null` 值
    fn asNull(): bool {
        return _.flex_type == FlexType.null_val;
    }

    /// 读取布尔值
    fn asBool(): bool? {
        if (_.flex_type != FlexType.bool_val) return null;
        const v = readUintWidth(_.data, _.offset, _.parent_width) ?? return null;
        return v != 0;
    }

    /// 读取有符号 64 位整数（支持直接 `int` 与间接 `indirect_int`）
    fn asInt(): i64? {
        if (_.flex_type == FlexType.int) {
            return readIntWidth(_.data, _.offset, _.parent_width);
        }
        if (_.flex_type == FlexType.indirect_int) {
            const target = _.indirectTarget() ?? return null;
            return readIntWidth(_.data, target, _.byte_width);
        }
        return null;
    }

    /// 读取无符号 64 位整数（支持直接 `uint` 与间接 `indirect_uint`）
    fn asUint(): u64? {
        if (_.flex_type == FlexType.uint) {
            return readUintWidth(_.data, _.offset, _.parent_width);
        }
        if (_.flex_type == FlexType.indirect_uint) {
            const target = _.indirectTarget() ?? return null;
            return readUintWidth(_.data, target, _.byte_width);
        }
        return null;
    }

    /// 读取浮点数（支持 32/64 位直接 `float` 与间接 `indirect_float`）
    fn asFloat(): f64? {
        if (_.flex_type == FlexType.float) {
            return readFloatWidth(_.data, _.offset, _.parent_width);
        }
        if (_.flex_type == FlexType.indirect_float) {
            const target = _.indirectTarget() ?? return null;
            return readFloatWidth(_.data, target, _.byte_width);
        }
        return null;
    }

    /// 零拷贝读取 UTF-8 字符串切片（前缀长度位于 `target - byte_width`）
    fn asString(): ([]const u8)? {
        if (_.flex_type != FlexType.string) return null;
        const target = _.indirectTarget() ?? return null;
        if (target < _.byte_width) return null;
        const len_u64 = readUintWidth(_.data, target - _.byte_width, _.byte_width) ?? return null;
        const len: usize = len_u64 as usize;
        if (len > _.data.len - target) return null;
        return _.data[target .. target + len];
    }

    /// 零拷贝读取 `\0` 结尾的 Key 字符串切片
    fn asKey(): ([]const u8)? {
        if (_.flex_type != FlexType.key) return null;
        const target = _.indirectTarget() ?? return null;
        let end = target;
        while (end < _.data.len && _.data[end] != 0) {
            end += 1;
        }
        if (end >= _.data.len) return null;
        return _.data[target..end];
    }

    /// 零拷贝读取二进制 Blob 切片
    fn asBlob(): ([]const u8)? {
        if (_.flex_type != FlexType.blob) return null;
        const target = _.indirectTarget() ?? return null;
        if (target < _.byte_width) return null;
        const len_u64 = readUintWidth(_.data, target - _.byte_width, _.byte_width) ?? return null;
        const len: usize = len_u64 as usize;
        if (len > _.data.len - target) return null;
        return _.data[target .. target + len];
    }

    /// 返回向量（多态 `vector`、定型向量或定长 2/3/4 向量）的长度
    fn vecLen(): usize? {
        if (fixed := fixedVectorLen(_.flex_type)) {
            const target = _.indirectTarget() ?? return null;
            if (fixed * _.byte_width > _.data.len - target) return null;
            return fixed;
        }
        const is_var_vec = _.flex_type == FlexType.vector
            || _.flex_type == FlexType.vector_int
            || _.flex_type == FlexType.vector_uint
            || _.flex_type == FlexType.vector_float
            || _.flex_type == FlexType.vector_key
            || _.flex_type == FlexType.vector_string_deprecated
            || _.flex_type == FlexType.vector_bool;
        if (!is_var_vec) return null;

        const target = _.indirectTarget() ?? return null;
        if (target < _.byte_width) return null;
        const len_u64 = readUintWidth(_.data, target - _.byte_width, _.byte_width) ?? return null;
        const len: usize = len_u64 as usize;
        if (_.byte_width > 0 && len > (_.data.len - target) / _.byte_width) return null;
        if (_.flex_type == FlexType.vector) {
            const elems_bytes: usize = len * _.byte_width;
            if (len > _.data.len - target - elems_bytes) return null;
        }
        return len;
    }

    /// 按下标零拷贝读取向量元素
    fn vecGet(idx: usize): FlexRef? {
        const len = _.vecLen() ?? return null;
        if (idx >= len) return null;

        const target = _.indirectTarget() ?? return null;
        const elem_off: usize = target + idx * _.byte_width;

        if (_.flex_type == FlexType.vector) {
            const types_off: usize = target + len * _.byte_width;
            if (types_off + idx >= _.data.len) return null;
            const unpacked = unpackFlexType(_.data[types_off + idx]) ?? return null;
            return FlexRef {
                data: _.data,
                offset: elem_off,
                parent_width: _.byte_width,
                byte_width: flexBitWidthBytes(unpacked.bit_width),
                flex_type: unpacked.flex_type,
            };
        }

        const elem_type = typedVectorElemType(_.flex_type) ?? return null;
        return FlexRef {
            data: _.data,
            offset: elem_off,
            parent_width: _.byte_width,
            byte_width: _.byte_width,
            flex_type: elem_type,
        };
    }

    /// 返回有序 Map 的键值对数量
    fn mapLen(): usize? {
        if (_.flex_type != FlexType.map) return null;
        const target = _.indirectTarget() ?? return null;
        if (target < _.byte_width * 3) return null;
        const len_u64 = readUintWidth(_.data, target - _.byte_width, _.byte_width) ?? return null;
        const len: usize = len_u64 as usize;
        if (len > (_.data.len - target) / _.byte_width) return null;
        const elems_bytes: usize = len * _.byte_width;
        if (len > _.data.len - target - elems_bytes) return null;
        return len;
    }

    /// 按下标读取 Map 的第 `idx` 个有序键名
    fn mapKeyAt(idx: usize): ([]const u8)? {
        const len = _.mapLen() ?? return null;
        if (idx >= len) return null;

        const target = _.indirectTarget() ?? return null;
        const keys_ptr: usize = target - _.byte_width * 3;
        const keys_bw_u64 = readUintWidth(_.data, target - _.byte_width * 2, _.byte_width) ?? return null;
        const keys_bw: usize = keys_bw_u64 as usize;
        if (!isValidByteWidth(keys_bw)) return null;

        const keys_rel = readUintWidth(_.data, keys_ptr, _.byte_width) ?? return null;
        if (keys_rel > (keys_ptr as u64)) return null;
        const keys_data: usize = keys_ptr - (keys_rel as usize);

        if (len > (_.data.len - keys_data) / keys_bw) return null;
        const key_ref = FlexRef {
            data: _.data,
            offset: keys_data + idx * keys_bw,
            parent_width: keys_bw,
            byte_width: 1,
            flex_type: FlexType.key,
        };
        return key_ref.asKey();
    }

    /// 按下标读取 Map 的第 `idx` 个值引用
    fn mapValueAt(idx: usize): FlexRef? {
        const len = _.mapLen() ?? return null;
        if (idx >= len) return null;

        const target = _.indirectTarget() ?? return null;
        const elem_off: usize = target + idx * _.byte_width;
        const types_off: usize = target + len * _.byte_width;
        if (types_off + idx >= _.data.len) return null;

        const unpacked = unpackFlexType(_.data[types_off + idx]) ?? return null;
        return FlexRef {
            data: _.data,
            offset: elem_off,
            parent_width: _.byte_width,
            byte_width: flexBitWidthBytes(unpacked.bit_width),
            flex_type: unpacked.flex_type,
        };
    }

    /// 在 Map 有序键向量上执行二分查找，返回匹配键对应的 `FlexRef`
    fn mapGet(key: []const u8): FlexRef? {
        const len = _.mapLen() ?? return null;
        if (len == 0) return null;

        let lo: usize = 0;
        let hi: usize = len;
        while (lo < hi) {
            const mid: usize = lo + ((hi - lo) / 2);
            const mid_key = _.mapKeyAt(mid) ?? return null;
            const cmp = compareBytes(mid_key, key);
            if (cmp == 0) {
                return _.mapValueAt(mid);
            }
            if (cmp < 0) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return null;
    }
}

/// 解析 FlexBuffers 缓冲区尾部 2 字节 `[root_packed_type, root_byte_width]` 并构造根 `FlexRef`
pub fn parseFlexRoot(data: []const u8): FlexRef? {
    if (data.len < 3) return null;

    const root_bw: usize = data[data.len - 1] as usize;
    if (!isValidByteWidth(root_bw)) return null;
    if (data.len < root_bw + 2) return null;

    const unpacked = unpackFlexType(data[data.len - 2]) ?? return null;
    const root_off: usize = data.len - 2 - root_bw;

    return FlexRef {
        data: data,
        offset: root_off,
        parent_width: root_bw,
        byte_width: flexBitWidthBytes(unpacked.bit_width),
        flex_type: unpacked.flex_type,
    };
}
