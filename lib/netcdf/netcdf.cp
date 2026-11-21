// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Unidata NetCDF CDF-1/CDF-2/CDF-5 Parser)

/// Unidata NetCDF 魔数、节标签与无限维度常量
pub const NC_MAGIC: []const u8 = "CDF";
pub const NC_ABSENT: u32 = 0;
pub const NC_DIMENSION: u32 = 10;
pub const NC_VARIABLE: u32 = 11;
pub const NC_ATTRIBUTE: u32 = 12;
pub const NC_UNLIMITED: u64 = 0;

/// NetCDF 容器格式版本枚举
pub enum NetcdfFormat: u8 {
    unknown = 0,
    classic = 1,
    offset64 = 2,
    cdf5 = 5,
}

/// NetCDF 标量数据类型枚举（含 CDF-5 扩展类型）
pub enum NetcdfType: u8 {
    unknown = 0,
    byte = 1,
    char = 2,
    short = 3,
    int = 4,
    float = 5,
    double = 6,
    ubyte = 7,
    ushort = 8,
    uint = 9,
    int64 = 10,
    uint64 = 11,
}

/// 返回 NetCDF 数据类型的单元素字节大小（未知类型返回 0）
pub fn netcdfTypeSize(t: NetcdfType): u8 {
    if (t == NetcdfType.byte || t == NetcdfType.char || t == NetcdfType.ubyte) return 1;
    if (t == NetcdfType.short || t == NetcdfType.ushort) return 2;
    if (t == NetcdfType.int || t == NetcdfType.float || t == NetcdfType.uint) return 4;
    if (t == NetcdfType.double || t == NetcdfType.int64 || t == NetcdfType.uint64) return 8;
    return 0;
}

/// 将原始 32 位大端类型码解码为 `NetcdfType`
pub fn decodeNetcdfType(code: u32): NetcdfType {
    if (code == 1) return NetcdfType.byte;
    if (code == 2) return NetcdfType.char;
    if (code == 3) return NetcdfType.short;
    if (code == 4) return NetcdfType.int;
    if (code == 5) return NetcdfType.float;
    if (code == 6) return NetcdfType.double;
    if (code == 7) return NetcdfType.ubyte;
    if (code == 8) return NetcdfType.ushort;
    if (code == 9) return NetcdfType.uint;
    if (code == 10) return NetcdfType.int64;
    if (code == 11) return NetcdfType.uint64;
    return NetcdfType.unknown;
}

/// NetCDF 维度定义零拷贝视图
pub struct NetcdfDim {
    name: []const u8,
    length: u64,
    is_unlimited: bool,
}

/// NetCDF 属性定义零拷贝视图
pub struct NetcdfAttr {
    name: []const u8,
    nc_type: NetcdfType,
    count: u64,
    raw_values: []const u8,
}

/// NetCDF 变量元数据零拷贝视图
pub struct NetcdfVar {
    name: []const u8,
    ndims: u32,
    nc_type: NetcdfType,
    vsize: u64,
    begin_offset: u64,
    attr_count: u32,
}

/// NetCDF 文件头摘要与三段零拷贝起止偏移
pub struct NetcdfHeader {
    format: NetcdfFormat,
    numrecs: u64,
    dim_count: u32,
    global_attr_count: u32,
    var_count: u32,
    dims_offset: usize,
    attrs_offset: usize,
    vars_offset: usize,
    header_end_offset: usize,
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

fn pad4(len: usize): usize {
    return (4 - (len & 3)) & 3;
}

fn readNonNeg(data: []const u8, pos: *usize, is_cdf5: bool): u64? {
    const p = pos.*;
    const width: usize = if (is_cdf5) 8 else 4;
    if (p > data.len || data.len - p < width) return null;
    const val: u64 = if (is_cdf5) readU64BE(data, p) else (readU32BE(data, p) as u64);
    pos.* = p + width;
    return val;
}

fn readAlignedName(data: []const u8, pos: *usize, is_cdf5: bool): ([]const u8)? {
    let len_u64: u64 = 0;
    if (v := readNonNeg(data, pos, is_cdf5)) {
        len_u64 = v;
    } else {
        return null;
    }

    const p = pos.*;
    if (len_u64 > ((data.len - p) as u64)) return null;
    const len = len_u64 as usize;
    const pad = pad4(len);
    if (pad > data.len - p - len) return null;

    const name = data[p .. p + len];
    pos.* = p + len + pad;
    return name;
}

fn readSectionHeader(data: []const u8, pos: *usize, expected_tag: u32, is_cdf5: bool): u32? {
    const p = pos.*;
    if (p > data.len || data.len - p < 4) return null;
    const tag = readU32BE(data, p);
    pos.* = p + 4;

    let count_u64: u64 = 0;
    if (v := readNonNeg(data, pos, is_cdf5)) {
        count_u64 = v;
    } else {
        return null;
    }

    if (tag == NC_ABSENT) {
        if (count_u64 != 0) return null;
        return 0;
    }
    if (tag != expected_tag) return null;
    if (count_u64 > 0xFFFFFFFF) return null;
    return count_u64 as u32;
}

fn parseOneAttr(data: []const u8, pos: *usize, is_cdf5: bool): NetcdfAttr? {
    let name: []const u8 = "";
    if (n := readAlignedName(data, pos, is_cdf5)) {
        name = n;
    } else {
        return null;
    }

    const p = pos.*;
    if (p > data.len || data.len - p < 4) return null;
    const nc_type = decodeNetcdfType(readU32BE(data, p));
    pos.* = p + 4;

    const elem_size = netcdfTypeSize(nc_type);
    if (elem_size == 0) return null;

    let count_u64: u64 = 0;
    if (c := readNonNeg(data, pos, is_cdf5)) {
        count_u64 = c;
    } else {
        return null;
    }

    const vp = pos.*;
    const avail: u64 = (data.len - vp) as u64;
    const es_u64: u64 = elem_size as u64;
    if (count_u64 > avail / es_u64) return null;

    const raw_len: usize = (count_u64 * es_u64) as usize;
    const pad = pad4(raw_len);
    if (pad > data.len - vp - raw_len) return null;

    const raw_values = data[vp .. vp + raw_len];
    pos.* = vp + raw_len + pad;

    return NetcdfAttr {
        name: name,
        nc_type: nc_type,
        count: count_u64,
        raw_values: raw_values,
    };
}

fn parseOneVar(data: []const u8, pos: *usize, format: NetcdfFormat): NetcdfVar? {
    const is_cdf5 = format == NetcdfFormat.cdf5;
    let name: []const u8 = "";
    if (n := readAlignedName(data, pos, is_cdf5)) {
        name = n;
    } else {
        return null;
    }

    let ndims_u64: u64 = 0;
    if (nd := readNonNeg(data, pos, is_cdf5)) {
        if (nd > 0xFFFFFFFF) return null;
        ndims_u64 = nd;
    } else {
        return null;
    }

    const dimid_width: usize = if (is_cdf5) 8 else 4;
    const p_dims = pos.*;
    const avail_dims: u64 = (data.len - p_dims) as u64;
    if (ndims_u64 > avail_dims / (dimid_width as u64)) return null;
    pos.* = p_dims + ((ndims_u64 * (dimid_width as u64)) as usize);

    // Variable-local attribute list
    let attr_count: u32 = 0;
    if (ac := readSectionHeader(data, pos, NC_ATTRIBUTE, is_cdf5)) {
        attr_count = ac;
    } else {
        return null;
    }
    let ai: u32 = 0;
    while (ai < attr_count) {
        if (parseOneAttr(data, pos, is_cdf5) == null) return null;
        ai += 1;
    }

    // nc_type (4B)
    const pt = pos.*;
    if (pt > data.len || data.len - pt < 4) return null;
    const nc_type = decodeNetcdfType(readU32BE(data, pt));
    if (nc_type == NetcdfType.unknown) return null;
    pos.* = pt + 4;

    // vsize (NON_NEG)
    let vsize: u64 = 0;
    if (vs := readNonNeg(data, pos, is_cdf5)) {
        vsize = vs;
    } else {
        return null;
    }

    // begin_offset: 4B for CDF-1, 8B for CDF-2 / CDF-5
    const pb = pos.*;
    const off_width: usize = if (format == NetcdfFormat.classic) 4 else 8;
    if (pb > data.len || data.len - pb < off_width) return null;
    const begin_offset: u64 = if (format == NetcdfFormat.classic) (readU32BE(data, pb) as u64) else readU64BE(data, pb);
    pos.* = pb + off_width;

    return NetcdfVar {
        name: name,
        ndims: ndims_u64 as u32,
        nc_type: nc_type,
        vsize: vsize,
        begin_offset: begin_offset,
        attr_count: attr_count,
    };
}

/// 探测 NetCDF 容器格式（CDF-1 Classic / CDF-2 64-bit Offset / CDF-5）
pub fn detectNetcdfFormat(data: []const u8): NetcdfFormat {
    if (data.len < 4) return NetcdfFormat.unknown;
    if (data[0] != 'C' || data[1] != 'D' || data[2] != 'F') return NetcdfFormat.unknown;
    if (data[3] == 1) return NetcdfFormat.classic;
    if (data[3] == 2) return NetcdfFormat.offset64;
    if (data[3] == 5) return NetcdfFormat.cdf5;
    return NetcdfFormat.unknown;
}

/// 判断字节切片是否为合法 NetCDF Classic / CDF-2 / CDF-5 魔数头
pub fn isNetcdf(data: []const u8): bool {
    return detectNetcdfFormat(data) != NetcdfFormat.unknown;
}

/// 解析 NetCDF 文件头并完整校验维度、全局属性与变量三段结构，记录零拷贝偏移
pub fn parseNetcdfHeader(data: []const u8): NetcdfHeader? {
    const format = detectNetcdfFormat(data);
    if (format == NetcdfFormat.unknown) return null;
    const is_cdf5 = format == NetcdfFormat.cdf5;

    let pos: usize = 4;
    let numrecs: u64 = 0;
    if (nr := readNonNeg(data, *pos, is_cdf5)) {
        numrecs = nr;
    } else {
        return null;
    }

    // 1. Dimensions section
    let dim_count: u32 = 0;
    if (dc := readSectionHeader(data, *pos, NC_DIMENSION, is_cdf5)) {
        dim_count = dc;
    } else {
        return null;
    }
    const dims_offset = pos;
    let di: u32 = 0;
    while (di < dim_count) {
        if (readAlignedName(data, *pos, is_cdf5) == null) return null;
        if (readNonNeg(data, *pos, is_cdf5) == null) return null;
        di += 1;
    }

    // 2. Global attributes section
    let global_attr_count: u32 = 0;
    if (ac := readSectionHeader(data, *pos, NC_ATTRIBUTE, is_cdf5)) {
        global_attr_count = ac;
    } else {
        return null;
    }
    const attrs_offset = pos;
    let ai: u32 = 0;
    while (ai < global_attr_count) {
        if (parseOneAttr(data, *pos, is_cdf5) == null) return null;
        ai += 1;
    }

    // 3. Variables section
    let var_count: u32 = 0;
    if (vc := readSectionHeader(data, *pos, NC_VARIABLE, is_cdf5)) {
        var_count = vc;
    } else {
        return null;
    }
    const vars_offset = pos;
    let vi: u32 = 0;
    while (vi < var_count) {
        if (parseOneVar(data, *pos, format) == null) return null;
        vi += 1;
    }

    return NetcdfHeader {
        format: format,
        numrecs: numrecs,
        dim_count: dim_count,
        global_attr_count: global_attr_count,
        var_count: var_count,
        dims_offset: dims_offset,
        attrs_offset: attrs_offset,
        vars_offset: vars_offset,
        header_end_offset: pos,
    };
}

/// 零堆分配 NetCDF 维度表迭代器
pub struct NetcdfDimIter {
    data: []const u8,
    pos: usize,
    index: u32 = 0,
    count: u32,
    is_cdf5: bool,
    had_error: bool = false,

    static fn init(data: []const u8, header: NetcdfHeader): NetcdfDimIter {
        return NetcdfDimIter {
            data: data,
            pos: header.dims_offset,
            index: 0,
            count: header.dim_count,
            is_cdf5: header.format == NetcdfFormat.cdf5,
            had_error: false,
        };
    }

    fn next(*_): NetcdfDim? {
        if (_.had_error || _.index >= _.count) return null;
        let p = _.pos;
        let name: []const u8 = "";
        if (n := readAlignedName(_.data, *p, _.is_cdf5)) {
            name = n;
        } else {
            _.had_error = true;
            return null;
        }
        let length: u64 = 0;
        if (l := readNonNeg(_.data, *p, _.is_cdf5)) {
            length = l;
        } else {
            _.had_error = true;
            return null;
        }
        _.pos = p;
        _.index += 1;
        return NetcdfDim {
            name: name,
            length: length,
            is_unlimited: length == NC_UNLIMITED,
        };
    }
}

/// 零堆分配 NetCDF 全局属性迭代器
pub struct NetcdfAttrIter {
    data: []const u8,
    pos: usize,
    index: u32 = 0,
    count: u32,
    is_cdf5: bool,
    had_error: bool = false,

    static fn init(data: []const u8, header: NetcdfHeader): NetcdfAttrIter {
        return NetcdfAttrIter {
            data: data,
            pos: header.attrs_offset,
            index: 0,
            count: header.global_attr_count,
            is_cdf5: header.format == NetcdfFormat.cdf5,
            had_error: false,
        };
    }

    fn next(*_): NetcdfAttr? {
        if (_.had_error || _.index >= _.count) return null;
        let p = _.pos;
        if (attr := parseOneAttr(_.data, *p, _.is_cdf5)) {
            _.pos = p;
            _.index += 1;
            return attr;
        }
        _.had_error = true;
        return null;
    }
}

/// 零堆分配 NetCDF 变量表迭代器
pub struct NetcdfVarIter {
    data: []const u8,
    pos: usize,
    index: u32 = 0,
    count: u32,
    format: NetcdfFormat,
    had_error: bool = false,

    static fn init(data: []const u8, header: NetcdfHeader): NetcdfVarIter {
        return NetcdfVarIter {
            data: data,
            pos: header.vars_offset,
            index: 0,
            count: header.var_count,
            format: header.format,
            had_error: false,
        };
    }

    fn next(*_): NetcdfVar? {
        if (_.had_error || _.index >= _.count) return null;
        let p = _.pos;
        if (v := parseOneVar(_.data, *p, _.format)) {
            _.pos = p;
            _.index += 1;
            return v;
        }
        _.had_error = true;
        return null;
    }
}
