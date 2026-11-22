// Origin: Pure-CP Stdlib (Zero-Alloc ASPRS LAS 1.2-1.4 LiDAR Point Cloud Parser) | Created: 2026-11-22

/// ASPRS LAS 1.2-1.4 魔数与头部大小常量
pub const LAS_MAGIC: []const u8 = "LASF";
pub const LAS_HEADER_SIZE_1_2: usize = 227;
pub const LAS_HEADER_SIZE_1_3: usize = 235;
pub const LAS_HEADER_SIZE_1_4: usize = 375;
pub const LAS_VLR_HEADER_SIZE: usize = 54;

/// LAS Public Header Block 零拷贝视图
pub struct LasHeader {
    version_major: u8,
    version_minor: u8,
    system_id: []const u8,
    generating_software: []const u8,
    header_size: u16,
    offset_to_point_data: u32,
    num_vlrs: u32,
    point_data_format: u8,
    is_laz_compressed: bool,
    point_record_len: u16,
    point_count: u64,
    x_scale: f64,
    y_scale: f64,
    z_scale: f64,
    x_offset: f64,
    y_offset: f64,
    z_offset: f64,
    x_max: f64,
    x_min: f64,
    y_max: f64,
    y_min: f64,
    z_max: f64,
    z_min: f64,
}

/// LAS 变长记录（Variable Length Record, VLR）零拷贝视图
pub struct LasVlr {
    user_id: []const u8,
    record_id: u16,
    description: []const u8,
    payload: []const u8,
}

/// 解码后的单条 3D LiDAR 点云记录
pub struct LasPoint {
    x: f64,
    y: f64,
    z: f64,
    intensity: u16,
    classification: u8,
    return_number: u8,
    num_returns: u8,
}

fn trimCString(raw: []const u8): []const u8 {
    let i: usize = 0;
    while (i < raw.len && raw[i] != 0) {
        i += 1;
    }
    return raw[0..i];
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

fn readI32LE(data: []const u8, off: usize): i32 {
    const u = readU32LE(data, off);
    if (u >= 0x80000000) {
        const inv = 0xFFFFFFFF - u;
        return -(inv as i32) - 1;
    }
    return u as i32;
}

fn readU64LE(data: []const u8, off: usize): u64 {
    const lo: u64 = readU32LE(data, off) as u64;
    const hi: u64 = readU32LE(data, off + 4) as u64;
    return lo | (hi << 32);
}

fn readF64LE(data: []const u8, off: usize): f64 {
    const bits: u64 = readU64LE(data, off);
    const val: f64 = @bitCast(bits);
    return val;
}

/// 校验字节切片是否以合法 LAS 1.x Public Header Block 开头
pub fn isLas(data: []const u8): bool {
    if (data.len < LAS_HEADER_SIZE_1_2) return false;
    if (data[0] != 'L' || data[1] != 'A' || data[2] != 'S' || data[3] != 'F') return false;
    if (data[24] != 1) return false;
    const header_size = readU16LE(data, 94);
    if ((header_size as usize) < LAS_HEADER_SIZE_1_2) return false;
    return true;
}

/// 解析 ASPRS LAS 1.2 / 1.3 / 1.4 Public Header Block（校验偏移、点记录长度与 LAZ 压缩位）
pub fn parseLasHeader(data: []const u8): LasHeader? {
    if (!isLas(data)) return null;

    const version_major = data[24];
    const version_minor = data[25];
    const header_size = readU16LE(data, 94);
    const offset_to_point_data = readU32LE(data, 96);
    const num_vlrs = readU32LE(data, 100);
    const raw_fmt = data[104];
    const is_laz = (raw_fmt & 0x80) != 0;
    const point_data_format = raw_fmt & 0x3F;
    const point_record_len = readU16LE(data, 105);

    if (point_record_len < 20) return null;
    if ((header_size as u32) > offset_to_point_data) return null;
    if ((offset_to_point_data as u64) > (data.len as u64)) return null;

    let point_count: u64 = 0;
    if (version_minor >= 4) {
        if ((header_size as usize) < LAS_HEADER_SIZE_1_4 || data.len < LAS_HEADER_SIZE_1_4) {
            return null;
        }
        point_count = readU64LE(data, 247);
    } else {
        point_count = readU32LE(data, 107) as u64;
    }

    return LasHeader {
        version_major: version_major,
        version_minor: version_minor,
        system_id: trimCString(data[26..58]),
        generating_software: trimCString(data[58..90]),
        header_size: header_size,
        offset_to_point_data: offset_to_point_data,
        num_vlrs: num_vlrs,
        point_data_format: point_data_format,
        is_laz_compressed: is_laz,
        point_record_len: point_record_len,
        point_count: point_count,
        x_scale: readF64LE(data, 131),
        y_scale: readF64LE(data, 139),
        z_scale: readF64LE(data, 147),
        x_offset: readF64LE(data, 155),
        y_offset: readF64LE(data, 163),
        z_offset: readF64LE(data, 171),
        x_max: readF64LE(data, 179),
        x_min: readF64LE(data, 187),
        y_max: readF64LE(data, 195),
        y_min: readF64LE(data, 203),
        z_max: readF64LE(data, 211),
        z_min: readF64LE(data, 219),
    };
}

/// 零堆分配 VLR（变长记录）迭代器，绝不越过 `offset_to_point_data`
pub struct LasVlrIter {
    data: []const u8,
    pos: usize,
    limit: usize,
    index: u32 = 0,
    count: u32,
    had_error: bool = false,

    static fn init(data: []const u8, header: LasHeader): LasVlrIter {
        const start = header.header_size as usize;
        const limit = header.offset_to_point_data as usize;
        if (start > limit || limit > data.len) {
            return LasVlrIter {
                data: data,
                pos: 0,
                limit: 0,
                index: 0,
                count: 0,
                had_error: true,
            };
        }
        return LasVlrIter {
            data: data,
            pos: start,
            limit: limit,
            index: 0,
            count: header.num_vlrs,
            had_error: false,
        };
    }

    fn next(*_): LasVlr? {
        if (_.had_error || _.index >= _.count) return null;
        if (_.limit - _.pos < LAS_VLR_HEADER_SIZE) {
            _.had_error = true;
            return null;
        }

        const off = _.pos;
        const user_id = trimCString(_.data[off + 2 .. off + 18]);
        const record_id = readU16LE(_.data, off + 18);
        const record_len = readU16LE(_.data, off + 20) as usize;
        const description = trimCString(_.data[off + 22 .. off + 54]);

        const payload_start = off + LAS_VLR_HEADER_SIZE;
        if (record_len > _.limit - payload_start) {
            _.had_error = true;
            return null;
        }

        const payload = _.data[payload_start .. payload_start + record_len];
        _.pos = payload_start + record_len;
        _.index += 1;

        return LasVlr {
            user_id: user_id,
            record_id: record_id,
            description: description,
            payload: payload,
        };
    }
}

/// 零堆分配 LAS 点云迭代器（支持 Format 0–5 与 Format 6–10；若 `is_laz_compressed` 为 true 则拒绝裸点迭代返回空）
pub struct LasPointIter {
    data: []const u8,
    pos: usize,
    index: u64 = 0,
    count: u64,
    record_len: usize,
    point_format: u8,
    x_scale: f64,
    y_scale: f64,
    z_scale: f64,
    x_offset: f64,
    y_offset: f64,
    z_offset: f64,
    had_error: bool = false,

    static fn init(data: []const u8, header: LasHeader): LasPointIter {
        if (header.is_laz_compressed) {
            return LasPointIter {
                data: data,
                pos: 0,
                index: 0,
                count: 0,
                record_len: header.point_record_len as usize,
                point_format: header.point_data_format,
                x_scale: header.x_scale,
                y_scale: header.y_scale,
                z_scale: header.z_scale,
                x_offset: header.x_offset,
                y_offset: header.y_offset,
                z_offset: header.z_offset,
                had_error: false,
            };
        }

        const start = header.offset_to_point_data as usize;
        const rlen = header.point_record_len as usize;
        if (start > data.len || rlen < 20) {
            return LasPointIter {
                data: data,
                pos: 0,
                index: 0,
                count: 0,
                record_len: 20,
                point_format: header.point_data_format,
                x_scale: 1.0,
                y_scale: 1.0,
                z_scale: 1.0,
                x_offset: 0.0,
                y_offset: 0.0,
                z_offset: 0.0,
                had_error: true,
            };
        }

        return LasPointIter {
            data: data,
            pos: start,
            index: 0,
            count: header.point_count,
            record_len: rlen,
            point_format: header.point_data_format,
            x_scale: header.x_scale,
            y_scale: header.y_scale,
            z_scale: header.z_scale,
            x_offset: header.x_offset,
            y_offset: header.y_offset,
            z_offset: header.z_offset,
            had_error: false,
        };
    }

    fn next(*_): LasPoint? {
        if (_.had_error || _.index >= _.count) return null;
        if (_.pos > _.data.len || _.record_len > _.data.len - _.pos) {
            _.had_error = true;
            return null;
        }

        const off = _.pos;
        const raw_x = readI32LE(_.data, off);
        const raw_y = readI32LE(_.data, off + 4);
        const raw_z = readI32LE(_.data, off + 8);
        const intensity = readU16LE(_.data, off + 12);

        const b14 = _.data[off + 14];
        let return_number: u8 = 0;
        let num_returns: u8 = 0;
        let classification: u8 = 0;

        if (_.point_format >= 6) {
            // LAS 1.4 Format 6..10: 4-bit return_number, 4-bit num_returns, full u8 classification at offset 16
            return_number = b14 & 0x0F;
            num_returns = (b14 >> 4) & 0x0F;
            classification = _.data[off + 16];
        } else {
            // LAS 1.2/1.3 Format 0..5: 3-bit return_number, 3-bit num_returns, 5-bit classification at offset 15
            return_number = b14 & 0x07;
            num_returns = (b14 >> 3) & 0x07;
            classification = _.data[off + 15] & 0x1F;
        }

        _.pos += _.record_len;
        _.index += 1;

        return LasPoint {
            x: (raw_x as f64) * _.x_scale + _.x_offset,
            y: (raw_y as f64) * _.y_scale + _.y_offset,
            z: (raw_z as f64) * _.z_scale + _.z_offset,
            intensity: intensity,
            classification: classification,
            return_number: return_number,
            num_returns: num_returns,
        };
    }
}
