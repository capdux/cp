// Origin: Pure-CP Stdlib (Zero-Alloc HDF5 Superblock v0-v3 & OHDR v2 Parser) | Created: 2026-11-22

/// HDF5 8 字节文件签名 `\x89HDF\r\n\x1a\n` 与核心常量
pub const HDF5_SIGNATURE: []const u8 = "\x89HDF\r\n\x1a\n";
pub const HDF5_UNDEF_ADDR_64: u64 = 0xFFFFFFFFFFFFFFFF;
pub const HDF5_OHDR_MAGIC: []const u8 = "OHDR";

/// HDF5 v2 对象头标志位
pub const HDF5_OHDR_FLAG_PHASE_CHANGE: u8 = 0x10;
pub const HDF5_OHDR_FLAG_TIMESTAMPS: u8 = 0x20;

/// HDF5 Superblock (v0 / v1 / v2 / v3) 统一解析结构
pub struct Hdf5Superblock {
    signature_offset: usize,
    version: u8,
    offset_size: u8,
    length_size: u8,
    group_leaf_node_k: u16,
    group_internal_node_k: u16,
    consistency_flags: u32,
    base_address: u64,
    free_space_address: u64,
    superblock_ext_address: u64,
    eof_address: u64,
    driver_info_address: u64,
    root_group_addr: u64,
    checksum: u32,
}

/// HDF5 Version 2 对象头（`OHDR`）零拷贝视图
pub struct Hdf5ObjHeaderV2 {
    version: u8,
    flags: u8,
    has_timestamps: bool,
    access_time: u32,
    mod_time: u32,
    change_time: u32,
    birth_time: u32,
    chunk0_size: u64,
    chunk0: []const u8,
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn isValidAddrSize(sz: u8): bool {
    return sz == 2 || sz == 4 || sz == 8;
}

fn readU16LE(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32)
        | ((data[off + 1] as u32) << 8)
        | ((data[off + 2] as u32) << 16)
        | ((data[off + 3] as u32) << 24);
}

fn readU64LE(data: []const u8, off: usize): u64 {
    const lo: u64 = readU32LE(data, off) as u64;
    const hi: u64 = readU32LE(data, off + 4) as u64;
    return lo | (hi << 32);
}

fn readVarU64LE(data: []const u8, off: usize, width: usize): u64 {
    if (width == 1) return data[off] as u64;
    if (width == 2) return readU16LE(data, off) as u64;
    if (width == 4) return readU32LE(data, off) as u64;
    return readU64LE(data, off);
}

/// 按 HDF5 规范依次在偏移 `0, 512, 1024, 2048, 4096, ...` 搜索 8 字节签名，跳过可选 User Block
pub fn findHdf5Signature(data: []const u8): usize? {
    if (data.len < 8) return null;
    if (strEq(data[0..8], HDF5_SIGNATURE)) return 0;

    const max_off = data.len - 8;
    let off: usize = 512;
    while (off <= max_off) {
        if (strEq(data[off .. off + 8], HDF5_SIGNATURE)) {
            return off;
        }
        if (off > max_off / 2) break;
        off = off * 2;
    }
    return null;
}

/// 校验字节切片是否包含合法 HDF5 8 字节签名（支持可选 User Block）
pub fn isHdf5(data: []const u8): bool {
    return findHdf5Signature(data) != null;
}

/// 解析 HDF5 Superblock v0 / v1 / v2 / v3（支持前置 User Block，校验 offset_size/length_size 与边界）
pub fn parseHdf5Superblock(data: []const u8): Hdf5Superblock? {
    let sig_off: usize = 0;
    if (found := findHdf5Signature(data)) {
        sig_off = found;
    } else {
        return null;
    }

    if (data.len - sig_off < 12) return null;
    const version = data[sig_off + 8];

    if (version == 0 || version == 1) {
        const fixed_len: usize = if (version == 0) 24 else 28;
        if (data.len - sig_off < fixed_len) return null;

        const offset_size = data[sig_off + 13];
        const length_size = data[sig_off + 14];
        if (!isValidAddrSize(offset_size) || !isValidAddrSize(length_size)) return null;

        const group_leaf_node_k = readU16LE(data, sig_off + 16);
        const group_internal_node_k = readU16LE(data, sig_off + 18);
        const consistency_flags = readU32LE(data, sig_off + 20);

        const o_len: usize = offset_size as usize;
        // 4 addresses + Root Group Symbol Table Entry (2*O + 24 bytes)
        const need_after_fixed: usize = (6 as usize) * o_len + (24 as usize);
        if (data.len - sig_off < fixed_len + need_after_fixed) return null;

        let p: usize = sig_off + fixed_len;
        const base_address = readVarU64LE(data, p, o_len);
        p += o_len;
        const free_space_address = readVarU64LE(data, p, o_len);
        p += o_len;
        const eof_address = readVarU64LE(data, p, o_len);
        p += o_len;
        const driver_info_address = readVarU64LE(data, p, o_len);
        p += o_len;

        // Root Group Symbol Table Entry: Link Name Offset (O) + Object Header Address (O) + ...
        p += o_len;
        const root_group_addr = readVarU64LE(data, p, o_len);

        return Hdf5Superblock {
            signature_offset: sig_off,
            version: version,
            offset_size: offset_size,
            length_size: length_size,
            group_leaf_node_k: group_leaf_node_k,
            group_internal_node_k: group_internal_node_k,
            consistency_flags: consistency_flags,
            base_address: base_address,
            free_space_address: free_space_address,
            superblock_ext_address: HDF5_UNDEF_ADDR_64,
            eof_address: eof_address,
            driver_info_address: driver_info_address,
            root_group_addr: root_group_addr,
            checksum: 0,
        };
    }

    if (version == 2 || version == 3) {
        const offset_size = data[sig_off + 9];
        const length_size = data[sig_off + 10];
        if (!isValidAddrSize(offset_size) || !isValidAddrSize(length_size)) return null;

        const consistency_flags: u32 = data[sig_off + 11] as u32;
        const o_len: usize = offset_size as usize;
        const total_need: usize = (12 as usize) + (4 as usize) * o_len + (4 as usize);
        if (data.len - sig_off < total_need) return null;

        let p: usize = sig_off + 12;
        const base_address = readVarU64LE(data, p, o_len);
        p += o_len;
        const superblock_ext_address = readVarU64LE(data, p, o_len);
        p += o_len;
        const eof_address = readVarU64LE(data, p, o_len);
        p += o_len;
        const root_group_addr = readVarU64LE(data, p, o_len);
        p += o_len;
        const checksum = readU32LE(data, p);

        return Hdf5Superblock {
            signature_offset: sig_off,
            version: version,
            offset_size: offset_size,
            length_size: length_size,
            group_leaf_node_k: 0,
            group_internal_node_k: 0,
            consistency_flags: consistency_flags,
            base_address: base_address,
            free_space_address: HDF5_UNDEF_ADDR_64,
            superblock_ext_address: superblock_ext_address,
            eof_address: eof_address,
            driver_info_address: HDF5_UNDEF_ADDR_64,
            root_group_addr: root_group_addr,
            checksum: checksum,
        };
    }

    return null;
}

/// 解析 HDF5 v2 对象头（`"OHDR"` + `version == 1` + 可选时间戳/属性阶段值 + 1/2/4/8B `chunk0_size` + 零拷贝 `chunk0` 切片）
pub fn parseHdf5ObjHeaderV2(data: []const u8, offset: usize): Hdf5ObjHeaderV2? {
    if (offset > data.len || data.len - offset < 6) return null;
    if (!strEq(data[offset .. offset + 4], HDF5_OHDR_MAGIC)) return null;

    const version = data[offset + 4];
    if (version != 1) return null;

    const flags = data[offset + 5];
    let p: usize = offset + 6;

    let has_timestamps = false;
    let access_time: u32 = 0;
    let mod_time: u32 = 0;
    let change_time: u32 = 0;
    let birth_time: u32 = 0;

    if ((flags & HDF5_OHDR_FLAG_TIMESTAMPS) != 0) {
        if (data.len - p < 16) return null;
        has_timestamps = true;
        access_time = readU32LE(data, p);
        mod_time = readU32LE(data, p + 4);
        change_time = readU32LE(data, p + 8);
        birth_time = readU32LE(data, p + 12);
        p += 16;
    }

    if ((flags & HDF5_OHDR_FLAG_PHASE_CHANGE) != 0) {
        if (data.len - p < 4) return null;
        p += 4;
    }

    const size_code = flags & 0x03;
    let chunk_size_width: usize = 1;
    if (size_code == 1) {
        chunk_size_width = 2;
    } else if (size_code == 2) {
        chunk_size_width = 4;
    } else if (size_code == 3) {
        chunk_size_width = 8;
    }

    if (data.len - p < chunk_size_width) return null;
    const chunk0_size = readVarU64LE(data, p, chunk_size_width);
    p += chunk_size_width;

    if (chunk0_size > ((data.len - p) as u64)) return null;
    const chunk0_len = chunk0_size as usize;

    return Hdf5ObjHeaderV2 {
        version: version,
        flags: flags,
        has_timestamps: has_timestamps,
        access_time: access_time,
        mod_time: mod_time,
        change_time: change_time,
        birth_time: birth_time,
        chunk0_size: chunk0_size,
        chunk0: data[p .. p + chunk0_len],
    };
}
