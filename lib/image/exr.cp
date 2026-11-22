// Origin: Pure-CP Stdlib (Zero-Alloc OpenEXR HDR Image Header & Attribute Parser) | Created: 2026-11-22

/// OpenEXR 4 字节魔数（Little-Endian `0x01312F76` = `20000630`；字节序列 `76 2F 31 01`）
pub const EXR_MAGIC: u32 = 20000630;

/// OpenEXR 版本字段标志位常量
pub const EXR_FLAG_TILED: u32 = 0x0200;
pub const EXR_FLAG_LONG_NAMES: u32 = 0x0400;
pub const EXR_FLAG_NON_IMAGE_PARTS: u32 = 0x0800;
pub const EXR_FLAG_MULTIPART: u32 = 0x1000;

/// OpenEXR 压缩算法枚举
pub enum ExrCompression: u8 {
    none = 0,
    rle = 1,
    zips = 2,
    zip = 3,
    piz = 4,
    pxr24 = 5,
    b44 = 6,
    b44a = 7,
    dwaa = 8,
    dwab = 9,
    unknown = 255,
}

/// OpenEXR `box2i` 二维整数包围框（闭区间 `[x_min..=x_max] x [y_min..=y_max]`）
pub struct ExrBox2i {
    x_min: i32,
    y_min: i32,
    x_max: i32,
    y_max: i32,

    /// 计算像素宽度（`x_max - x_min + 1`，非法倒置框返回 0）
    pub fn width(): u32 {
        if (_.x_max < _.x_min) return 0;
        const diff: i64 = (_.x_max as i64) - (_.x_min as i64) + 1;
        if (diff <= 0 || diff > 0xFFFFFFFF) return 0;
        return diff as u32;
    }

    /// 计算像素高度（`y_max - y_min + 1`，非法倒置框返回 0）
    pub fn height(): u32 {
        if (_.y_max < _.y_min) return 0;
        const diff: i64 = (_.y_max as i64) - (_.y_min as i64) + 1;
        if (diff <= 0 || diff > 0xFFFFFFFF) return 0;
        return diff as u32;
    }
}

/// 零拷贝 OpenEXR Header Attribute 视图
pub struct ExrAttr {
    name: []const u8,
    type_name: []const u8,
    size: u32,
    value: []const u8,
}

/// OpenEXR 图像头部元数据摘要
pub struct ExrHeader {
    version: u8,
    flags: u32,
    is_tiled: bool,
    has_long_names: bool,
    has_non_image_parts: bool,
    is_multipart: bool,
    compression: ExrCompression,
    data_window: ExrBox2i?,
    width: u32,
    height: u32,
    attr_count: usize,
    data_offset: usize,
}

fn readU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32)
        | ((data[off + 1] as u32) << 8)
        | ((data[off + 2] as u32) << 16)
        | ((data[off + 3] as u32) << 24);
}

fn readI32LE(data: []const u8, off: usize): i32 {
    const raw: u32 = readU32LE(data, off);
    const val: i32 = @bitCast(raw);
    return val;
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

/// 将单字节压缩编码映射为 `ExrCompression` 枚举
pub fn compressionFromByte(b: u8): ExrCompression {
    if (b == 0) return ExrCompression.none;
    if (b == 1) return ExrCompression.rle;
    if (b == 2) return ExrCompression.zips;
    if (b == 3) return ExrCompression.zip;
    if (b == 4) return ExrCompression.piz;
    if (b == 5) return ExrCompression.pxr24;
    if (b == 6) return ExrCompression.b44;
    if (b == 7) return ExrCompression.b44a;
    if (b == 8) return ExrCompression.dwaa;
    if (b == 9) return ExrCompression.dwab;
    return ExrCompression.unknown;
}

/// 快速探测缓冲区是否以 OpenEXR 魔数 `0x76 0x2F 0x31 0x01` 开头
pub fn isExr(data: []const u8): bool {
    if (data.len < 8) return false;
    return readU32LE(data, 0) == EXR_MAGIC;
}

/// 解析 16 字节 OpenEXR `box2i` 属性负载
pub fn parseExrBox2i(val: []const u8): ExrBox2i? {
    if (val.len != 16) return null;
    return ExrBox2i {
        x_min: readI32LE(val, 0),
        y_min: readI32LE(val, 4),
        x_max: readI32LE(val, 8),
        y_max: readI32LE(val, 12),
    };
}

/// 零堆分配 OpenEXR Header Attribute 流式迭代器
pub struct ExrAttrIter {
    data: []const u8,
    pos: usize = 8,
    finished: bool = false,
    had_error: bool = false,
    data_offset: usize = 0,

    pub static fn init(data: []const u8): ExrAttrIter {
        if (!isExr(data)) {
            return ExrAttrIter {
                data: data,
                pos: 0,
                finished: false,
                had_error: true,
                data_offset: 0,
            };
        }
        return ExrAttrIter {
            data: data,
            pos: 8,
            finished: false,
            had_error: false,
            data_offset: 0,
        };
    }

    fn readCString(*_): ([]const u8)? {
        const start = _.pos;
        while (_.pos < _.data.len) {
            if (_.data[_.pos] == 0) {
                const s = _.data[start.._.pos];
                _.pos += 1;
                return s;
            }
            _.pos += 1;
        }
        return null;
    }

    /// 读取下一条 Attribute；遇空属性名 `\0` 正常结束并记录 `data_offset`；遇截断/越界置 `had_error = true`
    pub fn next(*_): ExrAttr? {
        if (_.finished || _.had_error) return null;
        if (_.pos >= _.data.len) {
            _.had_error = true;
            return null;
        }

        // Empty name ('\0') terminates the header
        if (_.data[_.pos] == 0) {
            _.pos += 1;
            _.finished = true;
            _.data_offset = _.pos;
            return null;
        }

        let name: []const u8 = "";
        if (n := _.readCString()) {
            name = n;
        } else {
            _.had_error = true;
            return null;
        }

        let type_name: []const u8 = "";
        if (t := _.readCString()) {
            type_name = t;
        } else {
            _.had_error = true;
            return null;
        }

        if (_.data.len - _.pos < 4) {
            _.had_error = true;
            return null;
        }
        const raw_size = readU32LE(_.data, _.pos);
        _.pos += 4;

        // OpenEXR attribute size is a non-negative 32-bit signed integer
        if ((raw_size & 0x80000000) != 0) {
            _.had_error = true;
            return null;
        }

        const size_usize: usize = raw_size as usize;
        if (size_usize > _.data.len - _.pos) {
            _.had_error = true;
            return null;
        }

        const value = _.data[_.pos .. _.pos + size_usize];
        _.pos += size_usize;

        return ExrAttr {
            name: name,
            type_name: type_name,
            size: raw_size,
            value: value,
        };
    }
}

/// 解析 OpenEXR 完整头部（提取版本、标志位、`compression`、`dataWindow` 宽高、属性计数与像素数据偏移）
pub fn parseExrHeader(data: []const u8): ExrHeader? {
    if (!isExr(data)) return null;

    const ver_field = readU32LE(data, 4);
    const version: u8 = (ver_field & 0xFF) as u8;
    const flags: u32 = ver_field & 0xFFFFFF00;

    let compression = ExrCompression.unknown;
    let data_window: ExrBox2i? = null;
    let width: u32 = 0;
    let height: u32 = 0;
    let attr_count: usize = 0;

    let it = ExrAttrIter.init(data);
    while (attr := it.next()) {
        attr_count += 1;
        if (strEq(attr.name, "compression") && attr.value.len == 1) {
            compression = compressionFromByte(attr.value[0]);
        } else if (strEq(attr.name, "dataWindow") && strEq(attr.type_name, "box2i")) {
            if (box := parseExrBox2i(attr.value)) {
                data_window = box;
                width = box.width();
                height = box.height();
            }
        }
    }

    if (it.had_error || !it.finished) return null;

    return ExrHeader {
        version: version,
        flags: flags,
        is_tiled: (flags & EXR_FLAG_TILED) != 0,
        has_long_names: (flags & EXR_FLAG_LONG_NAMES) != 0,
        has_non_image_parts: (flags & EXR_FLAG_NON_IMAGE_PARTS) != 0,
        is_multipart: (flags & EXR_FLAG_MULTIPART) != 0,
        compression: compression,
        data_window: data_window,
        width: width,
        height: height,
        attr_count: attr_count,
        data_offset: it.data_offset,
    };
}
