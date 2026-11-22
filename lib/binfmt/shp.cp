// Origin: Pure-CP Stdlib (Zero-Alloc ESRI Shapefile .shp Parser) | Created: 2026-11-22

/// ESRI Shapefile 文件头魔数（4B Big-Endian = 9994）
pub const SHP_FILE_CODE: u32 = 9994;

/// ESRI Shapefile 版本号（4B Little-Endian = 1000）
pub const SHP_VERSION: u32 = 1000;

/// ESRI Shapefile 固定文件头大小（100 字节）与记录头大小（8 字节）
pub const SHP_HEADER_SIZE: usize = 100;
pub const SHP_RECORD_HEADER_SIZE: usize = 8;

/// ESRI Shapefile 几何类型枚举
pub enum ShpType: u32 {
    null_shape = 0,
    point = 1,
    polyline = 3,
    polygon = 5,
    multipoint = 8,
    point_z = 11,
    polyline_z = 13,
    polygon_z = 15,
    multipoint_z = 18,
    point_m = 21,
    polyline_m = 23,
    polygon_m = 25,
    multipoint_m = 28,
    multipatch = 31,
    unknown = 999,
}

/// 64 字节 Shapefile 空间外接包络盒（Bounding Box）
pub struct ShpBBox {
    x_min: f64,
    y_min: f64,
    x_max: f64,
    y_max: f64,
    z_min: f64,
    z_max: f64,
    m_min: f64,
    m_max: f64,
}

/// 100 字节混合端序 Shapefile 文件头解析结果
pub struct ShpHeader {
    file_code: u32,
    file_length_words: u32,
    file_length_bytes: usize,
    version: u32,
    shape_type: ShpType,
    bbox: ShpBBox,
}

/// 零拷贝 Shapefile 单条记录视图（8B BE 记录头 + LE 几何负载）
pub struct ShpRecord {
    record_number: u32,
    content_length_bytes: usize,
    shape_type: ShpType,
    content: []const u8,
}

/// 二维点几何（`type == 1`）
pub struct ShpPoint {
    x: f64,
    y: f64,
}

/// PolyLine / Polygon 头部视图（支持零拷贝 `partAt` 与 `pointAt` 随机访问）
pub struct ShpPolyHeader {
    shape_type: ShpType,
    bbox: ShpBBox,
    num_parts: u32,
    num_points: u32,
    parts_offset: usize,
    points_offset: usize,

    /// 读取第 `idx` 个子环/折线的起始点下标；越界返回 `null`
    fn partAt(content: []const u8, idx: u32): u32? {
        if (idx >= _.num_parts) return null;
        const off: usize = _.parts_offset + ((idx as usize) * 4);
        if (off > content.len || content.len - off < 4) return null;
        return readU32LE(content, off);
    }

    /// 零拷贝读取第 `idx` 个二维顶点坐标；越界返回 `null`
    fn pointAt(content: []const u8, idx: u32): ShpPoint? {
        if (idx >= _.num_points) return null;
        const off: usize = _.points_offset + ((idx as usize) * 16);
        if (off > content.len || content.len - off < 16) return null;
        return ShpPoint {
            x: readF64LE(content, off),
            y: readF64LE(content, off + 8),
        };
    }
}

/// 零堆分配 Shapefile 记录流式迭代器（从偏移 100 起逐条迭代至 `file_length_bytes`）
pub struct ShpRecordIter {
    data: []const u8,
    pos: usize = SHP_HEADER_SIZE,
    limit: usize = SHP_HEADER_SIZE,

    static fn init(data: []const u8): ShpRecordIter {
        if (hdr := parseShpHeader(data)) {
            return ShpRecordIter {
                data: data,
                pos: SHP_HEADER_SIZE,
                limit: hdr.file_length_bytes,
            };
        }
        return ShpRecordIter {
            data: data,
            pos: 0,
            limit: 0,
        };
    }

    /// 解析并返回下一条记录；到达末尾或遇到越界/畸形记录头时安全返回 `null`
    fn next(*_): ShpRecord? {
        if (_.pos >= _.limit) return null;
        if (_.limit - _.pos < SHP_RECORD_HEADER_SIZE) {
            _.pos = _.limit;
            return null;
        }

        const rec_num = readU32BE(_.data, _.pos);
        const content_words = readU32BE(_.data, _.pos + 4);
        const content_bytes_u64: u64 = (content_words as u64) * 2;
        const body_start: usize = _.pos + SHP_RECORD_HEADER_SIZE;

        if (content_bytes_u64 < 4 || content_bytes_u64 > ((_.limit - body_start) as u64)) {
            _.pos = _.limit;
            return null;
        }

        const content_bytes: usize = content_bytes_u64 as usize;
        const content = _.data[body_start .. body_start + content_bytes];
        const shape_type = rawToShpType(readU32LE(content, 0));
        _.pos = body_start + content_bytes;

        return ShpRecord {
            record_number: rec_num,
            content_length_bytes: content_bytes,
            shape_type: shape_type,
            content: content,
        };
    }
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
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
    return (hi << 32) | lo;
}

fn readF64LE(data: []const u8, off: usize): f64 {
    const bits = readU64LE(data, off);
    return @as(f64, @bitCast(bits));
}

fn rawToShpType(v: u32): ShpType {
    if (v == 0) return ShpType.null_shape;
    if (v == 1) return ShpType.point;
    if (v == 3) return ShpType.polyline;
    if (v == 5) return ShpType.polygon;
    if (v == 8) return ShpType.multipoint;
    if (v == 11) return ShpType.point_z;
    if (v == 13) return ShpType.polyline_z;
    if (v == 15) return ShpType.polygon_z;
    if (v == 18) return ShpType.multipoint_z;
    if (v == 21) return ShpType.point_m;
    if (v == 23) return ShpType.polyline_m;
    if (v == 25) return ShpType.polygon_m;
    if (v == 28) return ShpType.multipoint_m;
    if (v == 31) return ShpType.multipatch;
    return ShpType.unknown;
}

/// 解析 100 字节 ESRI Shapefile 混合端序头，校验 `file_code == 9994`、`version == 1000` 与 `file_length_words * 2 <= data.len`
pub fn parseShpHeader(data: []const u8): ShpHeader? {
    if (data.len < SHP_HEADER_SIZE) return null;

    const file_code = readU32BE(data, 0);
    if (file_code != SHP_FILE_CODE) return null;

    const file_length_words = readU32BE(data, 24);
    const file_length_bytes_u64: u64 = (file_length_words as u64) * 2;
    if (file_length_bytes_u64 < (SHP_HEADER_SIZE as u64) || file_length_bytes_u64 > (data.len as u64)) {
        return null;
    }

    const version = readU32LE(data, 28);
    if (version != SHP_VERSION) return null;

    const shape_type = rawToShpType(readU32LE(data, 32));
    const bbox = ShpBBox {
        x_min: readF64LE(data, 36),
        y_min: readF64LE(data, 44),
        x_max: readF64LE(data, 52),
        y_max: readF64LE(data, 60),
        z_min: readF64LE(data, 68),
        z_max: readF64LE(data, 76),
        m_min: readF64LE(data, 84),
        m_max: readF64LE(data, 92),
    };

    return ShpHeader {
        file_code: file_code,
        file_length_words: file_length_words,
        file_length_bytes: file_length_bytes_u64 as usize,
        version: version,
        shape_type: shape_type,
        bbox: bbox,
    };
}

/// 探测缓冲区是否为合法 ESRI Shapefile `.shp` 文件
pub fn isShp(data: []const u8): bool {
    return parseShpHeader(data) != null;
}

/// 解析 20 字节 Point 记录内容（`type == 1` + `x: f64` + `y: f64` LE）
pub fn parseShpPoint(content: []const u8): ShpPoint? {
    if (content.len < 20) return null;
    if (readU32LE(content, 0) != 1) return null;
    return ShpPoint {
        x: readF64LE(content, 4),
        y: readF64LE(content, 12),
    };
}

/// 解析 PolyLine (`type == 3`) 或 Polygon (`type == 5`) 记录头与索引偏移，严格校验 `parts` 和 `points` 边界
pub fn parseShpPoly(content: []const u8): ShpPolyHeader? {
    if (content.len < 44) return null;
    const raw_type = readU32LE(content, 0);
    if (raw_type != 3 && raw_type != 5) return null;

    const bbox = ShpBBox {
        x_min: readF64LE(content, 4),
        y_min: readF64LE(content, 12),
        x_max: readF64LE(content, 20),
        y_max: readF64LE(content, 28),
        z_min: 0.0,
        z_max: 0.0,
        m_min: 0.0,
        m_max: 0.0,
    };

    const num_parts = readU32LE(content, 36);
    const num_points = readU32LE(content, 40);

    const parts_bytes: u64 = (num_parts as u64) * 4;
    const points_bytes: u64 = (num_points as u64) * 16;
    const rem: u64 = (content.len - 44) as u64;
    if (parts_bytes > rem || points_bytes > rem - parts_bytes) return null;

    const parts_offset: usize = 44;
    const points_offset: usize = parts_offset + (parts_bytes as usize);

    return ShpPolyHeader {
        shape_type: rawToShpType(raw_type),
        bbox: bbox,
        num_parts: num_parts,
        num_points: num_points,
        parts_offset: parts_offset,
        points_offset: points_offset,
    };
}
