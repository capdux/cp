// Origin: Pure-CP Stdlib (Zero-Alloc GIMP XCF Layered Image Parser) | Created: 2026-11-22

/// GIMP XCF 9 字节公共魔数前缀（完整签名共 14B：`"gimp xcf "` + 4B 版本 + `'\0'`）
pub const XCF_MAGIC: []const u8 = "gimp xcf ";

/// 常用 GIMP XCF 属性 ID 常量
pub const PROP_END: u32 = 0;
pub const PROP_COLORMAP: u32 = 1;
pub const PROP_ACTIVE_LAYER: u32 = 2;
pub const PROP_OPACITY: u32 = 6;
pub const PROP_VISIBLE: u32 = 8;
pub const PROP_COMPRESSION: u32 = 17;
pub const PROP_RESOLUTION: u32 = 19;
pub const PROP_PARASITES: u32 = 21;
pub const PROP_UNIT: u32 = 22;
pub const PROP_PATHS: u32 = 23;

/// XCF 图像色彩模式枚举
pub enum XcfBaseType: u32 {
    rgb = 0,
    grayscale = 1,
    indexed = 2,
    unknown = 255,
}

/// XCF 像素瓦片压缩算法枚举（对应 `PROP_COMPRESSION` 载荷首字节）
pub enum XcfCompression: u8 {
    none = 0,
    rle = 1,
    zlib = 2,
    fractal = 3,
    unknown = 255,
}

/// XCF v4+ 图像位深与色彩空间精度枚举
pub enum XcfPrecision: u32 {
    unknown = 0,
    u8_linear = 100,
    u8_non_linear = 150,
    u16_linear = 200,
    u16_non_linear = 250,
    u32_linear = 300,
    u32_non_linear = 350,
    half_linear = 500,
    float_linear = 600,
    double_linear = 700,
}

/// 解析后的 XCF 文件头（支持 v000..v014、v4+ 精度与 v11+ 64-bit 偏移）
pub struct XcfHeader {
    version: u16,
    width: u32,
    height: u32,
    base_type: XcfBaseType,
    precision: XcfPrecision,
    uses_64bit_offsets: bool,
    props_offset: usize,
}

/// 零拷贝 XCF 属性条目视图
pub struct XcfProperty {
    prop_type: u32,
    payload: []const u8,
}

/// 零堆分配 XCF 属性表流式迭代器（遍历至 `PROP_END(0)` 后暴露 `layersListOffset()`）
pub struct XcfPropIter {
    data: []const u8,
    pos: usize = 0,
    ended: bool = false,

    static fn init(data: []const u8, props_offset: usize): XcfPropIter {
        const clamped = if (props_offset <= data.len) props_offset else data.len;
        return XcfPropIter {
            data: data,
            pos: clamped,
            ended: false,
        };
    }

    /// 读取下一条属性；遇到 `PROP_END`（`type==0 && size==0`）或截断越界时返回 `null`
    fn next(*_): XcfProperty? {
        if (_.ended || _.data.len - _.pos < 8) {
            _.ended = true;
            return null;
        }
        const prop_type = readU32BE(_.data, _.pos);
        const prop_size: usize = readU32BE(_.data, _.pos + 4) as usize;
        const body_start: usize = _.pos + 8;

        if (prop_type == PROP_END) {
            if (prop_size != 0) {
                _.ended = true;
                return null;
            }
            _.pos = body_start;
            _.ended = true;
            return null;
        }

        if (prop_size > _.data.len - body_start) {
            _.ended = true;
            return null;
        }

        const payload = _.data[body_start .. body_start + prop_size];
        _.pos = body_start + prop_size;
        return XcfProperty {
            prop_type: prop_type,
            payload: payload,
        };
    }

    /// 属性表结束后返回紧随其后的图层指针表起始偏移（未正常遇到 `PROP_END` 时返回 `null`）
    fn layersListOffset(): usize? {
        if (!_.ended) return null;
        return _.pos;
    }
}

/// 零堆分配 XCF 图层/通道偏移指针表迭代器（以 `0` 终止，支持 32-bit 与 64-bit 偏移）
pub struct XcfPointerIter {
    data: []const u8,
    pos: usize = 0,
    uses_64bit: bool = false,
    ended: bool = false,

    static fn init(data: []const u8, start_offset: usize, uses_64bit: bool): XcfPointerIter {
        const clamped = if (start_offset <= data.len) start_offset else data.len;
        return XcfPointerIter {
            data: data,
            pos: clamped,
            uses_64bit: uses_64bit,
            ended: false,
        };
    }

    /// 读取下一个非零有效偏移（严格校验 `< data.len`）；遇到 `0` 终止符或越界返回 `null`
    fn next(*_): usize? {
        if (_.ended) return null;
        const stride: usize = if (_.uses_64bit) 8 else 4;
        if (_.data.len - _.pos < stride) {
            _.ended = true;
            return null;
        }

        const raw: u64 = if (_.uses_64bit) readU64BE(_.data, _.pos) else (readU32BE(_.data, _.pos) as u64);
        _.pos += stride;

        if (raw == 0) {
            _.ended = true;
            return null;
        }
        if (raw >= (_.data.len as u64)) {
            _.ended = true;
            return null;
        }
        return raw as usize;
    }

    /// 返回指针表结束后下一区域（如通道指针表）的偏移
    fn nextOffset(): usize {
        return _.pos;
    }
}

/// 解析后的 XCF 图层头部信息（零拷贝剥离末尾 `'\0'` 的图层名）
pub struct XcfLayerInfo {
    width: u32,
    height: u32,
    layer_type: u32,
    name: []const u8,
    props_offset: usize,
    uses_64bit_offsets: bool,
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

fn readU64BE(data: []const u8, off: usize): u64 {
    const hi: u64 = readU32BE(data, off) as u64;
    const lo: u64 = readU32BE(data, off + 4) as u64;
    return (hi << 32) | lo;
}

fn rawToBaseType(v: u32): XcfBaseType {
    if (v == 0) return XcfBaseType.rgb;
    if (v == 1) return XcfBaseType.grayscale;
    if (v == 2) return XcfBaseType.indexed;
    return XcfBaseType.unknown;
}

/// 将 `PROP_COMPRESSION` 载荷首字节转换为 `XcfCompression` 枚举
pub fn parseXcfCompression(payload: []const u8): XcfCompression {
    if (payload.len < 1) return XcfCompression.unknown;
    const b = payload[0];
    if (b == 0) return XcfCompression.none;
    if (b == 1) return XcfCompression.rle;
    if (b == 2) return XcfCompression.zlib;
    if (b == 3) return XcfCompression.fractal;
    return XcfCompression.unknown;
}

fn rawToPrecision(v: u32): XcfPrecision {
    if (v == 100) return XcfPrecision.u8_linear;
    if (v == 150) return XcfPrecision.u8_non_linear;
    if (v == 200) return XcfPrecision.u16_linear;
    if (v == 250) return XcfPrecision.u16_non_linear;
    if (v == 300) return XcfPrecision.u32_linear;
    if (v == 350) return XcfPrecision.u32_non_linear;
    if (v == 500) return XcfPrecision.half_linear;
    if (v == 600) return XcfPrecision.float_linear;
    if (v == 700) return XcfPrecision.double_linear;
    return XcfPrecision.unknown;
}

fn parseVersionTag(data: []const u8): u16? {
    if (data.len < 14) return null;
    if (data[0] != 'g' || data[1] != 'i' || data[2] != 'm' || data[3] != 'p'
        || data[4] != ' ' || data[5] != 'x' || data[6] != 'c' || data[7] != 'f' || data[8] != ' ') {
        return null;
    }
    if (data[13] != 0) return null;

    // "file\0" => version 0
    if (data[9] == 'f' && data[10] == 'i' && data[11] == 'l' && data[12] == 'e') {
        return 0;
    }

    // "v001\0" .. "v014\0"
    if (data[9] != 'v') return null;
    const d0 = data[10];
    const d1 = data[11];
    const d2 = data[12];
    if (d0 < '0' || d0 > '9' || d1 < '0' || d1 > '9' || d2 < '0' || d2 > '9') {
        return null;
    }
    const ver: u16 = ((d0 - '0') as u16) * 100 + ((d1 - '0') as u16) * 10 + ((d2 - '0') as u16);
    if (ver == 0 || ver > 14) return null;
    return ver;
}

/// 探测缓冲区是否以合法 GIMP XCF v000..v014 14 字节魔数+版本串开头
pub fn isXcf(data: []const u8): bool {
    return parseVersionTag(data) != null;
}

/// 解析 GIMP XCF 文件头（含 v4+ 精度与 v11+ 64-bit 偏移判定）
pub fn parseXcfHeader(data: []const u8): XcfHeader? {
    const version = parseVersionTag(data) ?? return null;
    const min_len: usize = if (version >= 4) 30 else 26;
    if (data.len < min_len) return null;

    const width = readU32BE(data, 14);
    const height = readU32BE(data, 18);
    const base_type = rawToBaseType(readU32BE(data, 22));
    const precision = if (version >= 4) rawToPrecision(readU32BE(data, 26)) else XcfPrecision.u8_non_linear;

    return XcfHeader {
        version: version,
        width: width,
        height: height,
        base_type: base_type,
        precision: precision,
        uses_64bit_offsets: version >= 11,
        props_offset: min_len,
    };
}

/// 在指定偏移处解析 XCF 图层头（宽高、图层类型与剥离末尾 `'\0'` 的 UTF-8 图层名）
pub fn parseXcfLayerHeader(data: []const u8, offset: usize, uses_64bit: bool): XcfLayerInfo? {
    if (offset > data.len || data.len - offset < 16) return null;

    const width = readU32BE(data, offset);
    const height = readU32BE(data, offset + 4);
    const layer_type = readU32BE(data, offset + 8);
    const name_len: usize = readU32BE(data, offset + 12) as usize;

    const name_start: usize = offset + 16;
    if (name_len > data.len - name_start) return null;
    const props_offset: usize = name_start + name_len;

    let name_end: usize = props_offset;
    if (name_end > name_start && data[name_end - 1] == 0) {
        name_end -= 1;
    }

    return XcfLayerInfo {
        width: width,
        height: height,
        layer_type: layer_type,
        name: data[name_start..name_end],
        props_offset: props_offset,
        uses_64bit_offsets: uses_64bit,
    };
}
