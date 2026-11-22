// Origin: Pure-CP Stdlib (Zero-Alloc Khronos KTX 1.1 & KTX2 Texture Container Parser) | Created: 2026-11-22

/// Khronos KTX 容器版本
pub enum KtxVersion {
    ktx1,
    ktx2,
    unknown,
}

/// KTX 1.1 64 字节文件头（自动适配大小端）
pub struct Ktx1Header {
    is_big_endian: bool,
    gl_type: u32,
    gl_type_size: u32,
    gl_format: u32,
    gl_internal_format: u32,
    gl_base_internal_format: u32,
    pixel_width: u32,
    pixel_height: u32,
    pixel_depth: u32,
    array_elements: u32,
    faces: u32,
    mipmap_levels: u32,
    bytes_of_key_value_data: u32,

    /// 跳过 64 字节头与 Key-Value 元数据后的首个 Mipmap 图像数据起始偏移
    fn dataOffset(): usize {
        return 64 + usize(_.bytes_of_key_value_data);
    }
}

/// KTX 2.0 80 字节小端文件头
pub struct Ktx2Header {
    vk_format: u32,
    type_size: u32,
    pixel_width: u32,
    pixel_height: u32,
    pixel_depth: u32,
    layer_count: u32,
    face_count: u32,
    level_count: u32,
    supercompression_scheme: u32,
    dfd_byte_offset: u32,
    dfd_byte_length: u32,
    kvd_byte_offset: u32,
    kvd_byte_length: u32,
    sgd_byte_offset: u64,
    sgd_byte_length: u64,

    /// 有效 Mipmap 层数（KTX2 规范：`level_count == 0` 表示 1 级基础纹理）
    fn effectiveLevels(): u32 {
        return if (_.level_count == 0) 1 else _.level_count;
    }
}

/// KTX 2.0 24 字节 Level Index 条目
pub struct Ktx2Level {
    byte_offset: u64,
    byte_length: u64,
    uncompressed_byte_length: u64,

    /// 带边界与溢出检查返回该 Mip 层的零拷贝原始字节切片
    fn payload(data: []const u8): ([]const u8)? {
        const total = u64(data.len);
        if (_.byte_offset > total || _.byte_length > total - _.byte_offset) {
            return null;
        }
        const off = usize(_.byte_offset);
        const len = usize(_.byte_length);
        return data[off .. off + len];
    }
}

const KTX1_HEADER_SIZE: usize = 64;
const KTX2_HEADER_SIZE: usize = 80;
const KTX2_LEVEL_ENTRY_SIZE: usize = 24;

fn readU32Le(data: []const u8, pos: usize): u32 {
    return u32(data[pos])
        | (u32(data[pos + 1]) << 8)
        | (u32(data[pos + 2]) << 16)
        | (u32(data[pos + 3]) << 24);
}

fn readU32Be(data: []const u8, pos: usize): u32 {
    return (u32(data[pos]) << 24)
        | (u32(data[pos + 1]) << 16)
        | (u32(data[pos + 2]) << 8)
        | u32(data[pos + 3]);
}

fn readU32Endian(data: []const u8, pos: usize, big_endian: bool): u32 {
    return if (big_endian) readU32Be(data, pos) else readU32Le(data, pos);
}

fn readU64Le(data: []const u8, pos: usize): u64 {
    const lo = u64(readU32Le(data, pos));
    const hi = u64(readU32Le(data, pos + 4));
    return lo | (hi << 32);
}

/// 校验 12 字节 KTX 1.1 魔数 `«KTX 11»\r\n\x1a\n`
pub fn isKtx1(data: []const u8): bool {
    if (data.len < 12) return false;
    return data[0] == 0xAB
        && data[1] == 0x4B
        && data[2] == 0x54
        && data[3] == 0x58
        && data[4] == 0x20
        && data[5] == 0x31
        && data[6] == 0x31
        && data[7] == 0xBB
        && data[8] == 0x0D
        && data[9] == 0x0A
        && data[10] == 0x1A
        && data[11] == 0x0A;
}

/// 校验 12 字节 KTX 2.0 魔数 `«KTX 20»\r\n\x1a\n`
pub fn isKtx2(data: []const u8): bool {
    if (data.len < 12) return false;
    return data[0] == 0xAB
        && data[1] == 0x4B
        && data[2] == 0x54
        && data[3] == 0x58
        && data[4] == 0x20
        && data[5] == 0x32
        && data[6] == 0x30
        && data[7] == 0xBB
        && data[8] == 0x0D
        && data[9] == 0x0A
        && data[10] == 0x1A
        && data[11] == 0x0A;
}

/// 检测 KTX 容器版本（`ktx1` / `ktx2` / `unknown`）
pub fn detectKtxVersion(data: []const u8): KtxVersion {
    if (isKtx2(data)) return KtxVersion.ktx2;
    if (isKtx1(data)) return KtxVersion.ktx1;
    return KtxVersion.unknown;
}

/// 解析 KTX 1.1 64 字节头（自动识别 `0x04030201` 大小端，校验 KVD 偏移不越界）
pub fn parseKtx1Header(data: []const u8): Ktx1Header? {
    if (data.len < KTX1_HEADER_SIZE || !isKtx1(data)) {
        return null;
    }

    const endian_le = readU32Le(data, 12);
    let is_be = false;
    if (endian_le == 0x04030201) {
        is_be = false;
    } else if (endian_le == 0x01020304) {
        is_be = true;
    } else {
        return null;
    }

    const kvd_bytes = readU32Endian(data, 60, is_be);
    if (usize(kvd_bytes) > data.len - KTX1_HEADER_SIZE) {
        return null;
    }

    return Ktx1Header {
        is_big_endian: is_be,
        gl_type: readU32Endian(data, 16, is_be),
        gl_type_size: readU32Endian(data, 20, is_be),
        gl_format: readU32Endian(data, 24, is_be),
        gl_internal_format: readU32Endian(data, 28, is_be),
        gl_base_internal_format: readU32Endian(data, 32, is_be),
        pixel_width: readU32Endian(data, 36, is_be),
        pixel_height: readU32Endian(data, 40, is_be),
        pixel_depth: readU32Endian(data, 44, is_be),
        array_elements: readU32Endian(data, 48, is_be),
        faces: readU32Endian(data, 52, is_be),
        mipmap_levels: readU32Endian(data, 56, is_be),
        bytes_of_key_value_data: kvd_bytes,
    };
}

fn spanInBounds(offset: u64, length: u64, total: u64): bool {
    if (length == 0) return true;
    return offset <= total && length <= total - offset;
}

/// 解析 KTX 2.0 80 字节小端头（校验 DFD / KVD / SGD 索引区间不越界）
pub fn parseKtx2Header(data: []const u8): Ktx2Header? {
    if (data.len < KTX2_HEADER_SIZE || !isKtx2(data)) {
        return null;
    }

    const dfd_off = readU32Le(data, 48);
    const dfd_len = readU32Le(data, 52);
    const kvd_off = readU32Le(data, 56);
    const kvd_len = readU32Le(data, 60);
    const sgd_off = readU64Le(data, 64);
    const sgd_len = readU64Le(data, 72);

    const total = u64(data.len);
    if (!spanInBounds(u64(dfd_off), u64(dfd_len), total)) return null;
    if (!spanInBounds(u64(kvd_off), u64(kvd_len), total)) return null;
    if (!spanInBounds(sgd_off, sgd_len, total)) return null;

    return Ktx2Header {
        vk_format: readU32Le(data, 12),
        type_size: readU32Le(data, 16),
        pixel_width: readU32Le(data, 20),
        pixel_height: readU32Le(data, 24),
        pixel_depth: readU32Le(data, 28),
        layer_count: readU32Le(data, 32),
        face_count: readU32Le(data, 36),
        level_count: readU32Le(data, 40),
        supercompression_scheme: readU32Le(data, 44),
        dfd_byte_offset: dfd_off,
        dfd_byte_length: dfd_len,
        kvd_byte_offset: kvd_off,
        kvd_byte_length: kvd_len,
        sgd_byte_offset: sgd_off,
        sgd_byte_length: sgd_len,
    };
}

/// 零拷贝读取 KTX2 第 `level_idx` 级 Mipmap Level Index 条目（严格校验表项与数据负载边界）
pub fn readKtx2Level(data: []const u8, level_idx: u32): Ktx2Level? {
    let hdr = Ktx2Header {
        vk_format: 0,
        type_size: 0,
        pixel_width: 0,
        pixel_height: 0,
        pixel_depth: 0,
        layer_count: 0,
        face_count: 0,
        level_count: 0,
        supercompression_scheme: 0,
        dfd_byte_offset: 0,
        dfd_byte_length: 0,
        kvd_byte_offset: 0,
        kvd_byte_length: 0,
        sgd_byte_offset: 0,
        sgd_byte_length: 0,
    };
    if (h := parseKtx2Header(data)) {
        hdr = h;
    } else {
        return null;
    }

    const total_levels = hdr.effectiveLevels();
    if (level_idx >= total_levels) {
        return null;
    }

    const entry_off_u64 = u64(KTX2_HEADER_SIZE) + u64(level_idx) * u64(KTX2_LEVEL_ENTRY_SIZE);
    const total_u64 = u64(data.len);
    if (entry_off_u64 > total_u64 || u64(KTX2_LEVEL_ENTRY_SIZE) > total_u64 - entry_off_u64) {
        return null;
    }

    const pos = usize(entry_off_u64);
    const byte_offset = readU64Le(data, pos);
    const byte_length = readU64Le(data, pos + 8);
    const uncompressed_byte_length = readU64Le(data, pos + 16);

    if (byte_offset > total_u64 || byte_length > total_u64 - byte_offset) {
        return null;
    }

    return Ktx2Level {
        byte_offset: byte_offset,
        byte_length: byte_length,
        uncompressed_byte_length: uncompressed_byte_length,
    };
}
