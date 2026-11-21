// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc DirectDraw Surface DDS Texture Parser)

/// 32 字节 `DDS_PIXELFORMAT` 像素格式描述
pub struct DdsPixelFormat {
    flags: u32,
    fourcc: [4]u8,
    rgb_bit_count: u32,
    r_mask: u32,
    g_mask: u32,
    b_mask: u32,
    a_mask: u32,

    /// 是否使用 FourCC 压缩格式（`DDPF_FOURCC = 0x4`）
    fn isFourcc(): bool {
        return (_.flags & 0x4) != 0;
    }

    /// 是否带 DX10 扩展头（`isFourcc()` 且 `fourcc == "DX10"`）
    fn isDx10(): bool {
        return _.isFourcc()
            && _.fourcc[0] == 'D'
            && _.fourcc[1] == 'X'
            && _.fourcc[2] == '1'
            && _.fourcc[3] == '0';
    }
}

/// 20 字节 `DDS_HEADER_DXT10` 扩展头
pub struct DdsDx10Header {
    dxgi_format: u32,
    resource_dimension: u32,
    misc_flag: u32,
    array_size: u32,
    misc_flags2: u32,
}

/// 128 字节（含 DX10 时 148 字节）`DDS_HEADER` 纹理头
pub struct DdsHeader {
    flags: u32,
    height: u32,
    width: u32,
    pitch_or_linear_size: u32,
    depth: u32,
    mip_map_count: u32,
    pixel_format: DdsPixelFormat,
    caps: u32,
    caps2: u32,
    dx10: DdsDx10Header?,
    data_offset: usize,

    /// 是否为立方体贴图（`DDSCAPS2_CUBEMAP = 0x200`）
    fn isCubemap(): bool {
        return (_.caps2 & 0x200) != 0;
    }

    /// 是否为 3D 体积纹理（`DDSCAPS2_VOLUME = 0x200000`）
    fn isVolume(): bool {
        return (_.caps2 & 0x200000) != 0;
    }
}

const DDS_HEADER_TOTAL: usize = 128;
const DDS_DX10_TOTAL: usize = 148;

fn readU32Le(data: []const u8, pos: usize): u32 {
    return u32(data[pos])
        | (u32(data[pos + 1]) << 8)
        | (u32(data[pos + 2]) << 16)
        | (u32(data[pos + 3]) << 24);
}

/// 快速校验 `"DDS "`（`0x20534444`）4 字节魔数
pub fn isDds(data: []const u8): bool {
    return data.len >= 4
        && data[0] == 'D'
        && data[1] == 'D'
        && data[2] == 'S'
        && data[3] == ' ';
}

/// 解析 128 字节 DDS 头及可选 20 字节 DX10 扩展头；魔数、结构大小或长度非法时返回 `null`
pub fn parseDdsHeader(data: []const u8): DdsHeader? {
    if (data.len < DDS_HEADER_TOTAL || !isDds(data)) {
        return null;
    }

    const dw_size = readU32Le(data, 4);
    if (dw_size != 124) {
        return null;
    }

    const flags = readU32Le(data, 8);
    const height = readU32Le(data, 12);
    const width = readU32Le(data, 16);
    const pitch_or_linear_size = readU32Le(data, 20);
    const depth = readU32Le(data, 24);
    const mip_map_count = readU32Le(data, 28);

    const pf_size = readU32Le(data, 76);
    if (pf_size != 32) {
        return null;
    }

    const pf_flags = readU32Le(data, 80);
    const fourcc: [4]u8 = [data[84], data[85], data[86], data[87]];
    const rgb_bit_count = readU32Le(data, 88);
    const r_mask = readU32Le(data, 92);
    const g_mask = readU32Le(data, 96);
    const b_mask = readU32Le(data, 100);
    const a_mask = readU32Le(data, 104);

    const pf = DdsPixelFormat {
        flags: pf_flags,
        fourcc: fourcc,
        rgb_bit_count: rgb_bit_count,
        r_mask: r_mask,
        g_mask: g_mask,
        b_mask: b_mask,
        a_mask: a_mask,
    };

    const caps = readU32Le(data, 108);
    const caps2 = readU32Le(data, 112);

    let dx10: DdsDx10Header? = null;
    let data_offset: usize = DDS_HEADER_TOTAL;

    if (pf.isDx10()) {
        if (data.len < DDS_DX10_TOTAL) {
            return null;
        }
        dx10 = DdsDx10Header {
            dxgi_format: readU32Le(data, 128),
            resource_dimension: readU32Le(data, 132),
            misc_flag: readU32Le(data, 136),
            array_size: readU32Le(data, 140),
            misc_flags2: readU32Le(data, 144),
        };
        data_offset = DDS_DX10_TOTAL;
    }

    return DdsHeader {
        flags: flags,
        height: height,
        width: width,
        pitch_or_linear_size: pitch_or_linear_size,
        depth: depth,
        mip_map_count: mip_map_count,
        pixel_format: pf,
        caps: caps,
        caps2: caps2,
        dx10: dx10,
        data_offset: data_offset,
    };
}

/// 按 `max(1, (w + 3) / 4) * max(1, (h + 3) / 4) * block_bytes` 防溢出计算 BCn/DXTn 单个 Mip 层字节数
pub fn calcDdsMipSize(width: u32, height: u32, block_bytes: u32): u64 {
    if (block_bytes == 0) {
        return 0;
    }
    const bw: u64 = if (width == 0) 1 else (u64(width) + 3) / 4;
    const bh: u64 = if (height == 0) 1 else (u64(height) + 3) / 4;
    const blocks = bw * bh;
    const bb = u64(block_bytes);
    const max_u64: u64 = 0xFFFF_FFFF_FFFF_FFFF;
    if (blocks > max_u64 / bb) {
        return max_u64;
    }
    return blocks * bb;
}
