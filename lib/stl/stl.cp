// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc 3D STL Binary & ASCII Mesh Parser)

/// 3D STL 文件格式枚举
pub enum StlFormat: u8 {
    binary = 0,
    ascii = 1,
    unknown = 2,
}

/// 3D 单精度浮点向量（法线或顶点坐标）
pub struct StlVec3 {
    x: f32,
    y: f32,
    z: f32,
}

/// 3D STL 三角面片（50 字节二进制记录）
pub struct StlTriangle {
    normal: StlVec3,
    v1: StlVec3,
    v2: StlVec3,
    v3: StlVec3,
    attr_byte_count: u16,
}

/// 二进制 STL 84 字节头部（80 字节头文本 + 4 字节小端三角面片计数）
pub struct StlBinaryHeader {
    header_bytes: [80]u8,
    triangle_count: u32,
}

/// 3D 轴对齐包围盒（AABB）统计结果
pub struct StlBounds {
    min: StlVec3,
    max: StlVec3,
    triangle_count: usize,
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

fn readF32LE(data: []const u8, off: usize): f32 {
    const bits: u32 = readU32LE(data, off);
    const val: f32 = @bitCast(bits);
    return val;
}

fn readVec3LE(data: []const u8, off: usize): StlVec3 {
    return StlVec3 {
        x: readF32LE(data, off),
        y: readF32LE(data, off + 4),
        z: readF32LE(data, off + 8),
    };
}

fn writeU16LE(out: []u8, off: usize, v: u16) {
    out[off] = (v & 0xFF) as u8;
    out[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeU32LE(out: []u8, off: usize, v: u32) {
    out[off] = (v & 0xFF) as u8;
    out[off + 1] = ((v >> 8) & 0xFF) as u8;
    out[off + 2] = ((v >> 16) & 0xFF) as u8;
    out[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeF32LE(out: []u8, off: usize, v: f32) {
    const bits: u32 = @bitCast(v);
    writeU32LE(out, off, bits);
}

fn writeVec3LE(out: []u8, off: usize, v: StlVec3) {
    writeF32LE(out, off, v.x);
    writeF32LE(out, off + 4, v.y);
    writeF32LE(out, off + 8, v.z);
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn strEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

/// 探测 STL 格式：优先按 `84 + 50 * N == data.len` 精确判定二进制，避免头部以 `"solid "` 开头的二进制 STL 被误判为 ASCII
pub fn detectStlFormat(data: []const u8): StlFormat {
    if (data.len >= 84) {
        const tri_count: u64 = readU32LE(data, 80) as u64;
        const expected: u64 = tri_count * 50 + 84;
        if (expected == (data.len as u64)) {
            return StlFormat.binary;
        }
    }

    let i: usize = 0;
    while (i < data.len && isSpace(data[i])) {
        i += 1;
    }
    if (data.len - i >= 5 && strEqlIgnoreCase(data[i .. i + 5], "solid")) {
        if (i + 5 == data.len || isSpace(data[i + 5])) {
            return StlFormat.ascii;
        }
    }

    return StlFormat.unknown;
}

/// 解析 84 字节二进制 STL 头部并严格防溢出校验 `84 + triangle_count * 50 <= data.len`
pub fn parseStlBinaryHeader(data: []const u8): StlBinaryHeader? {
    if (data.len < 84) return null;
    const tri_count = readU32LE(data, 80);
    const max_tris: usize = (data.len - 84) / 50;
    if ((tri_count as usize) > max_tris) return null;

    let hdr: [80]u8 = undefined;
    let i: usize = 0;
    while (i < 80) {
        hdr[i] = data[i];
        i += 1;
    }

    return StlBinaryHeader {
        header_bytes: hdr,
        triangle_count: tri_count,
    };
}

/// 零堆分配二进制 STL 三角面片流式迭代器
pub struct StlBinaryIter {
    data: []const u8,
    pos: usize = 84,
    remaining: u32 = 0,

    pub static fn init(data: []const u8): StlBinaryIter {
        if (hdr := parseStlBinaryHeader(data)) {
            return StlBinaryIter {
                data: data,
                pos: 84,
                remaining: hdr.triangle_count,
            };
        }
        return StlBinaryIter {
            data: data,
            pos: 0,
            remaining: 0,
        };
    }

    /// 小端 IEEE 754 `f32` 解码下一条 50 字节三角面片；到达末尾时返回 `null`
    pub fn next(*_): StlTriangle? {
        if (_.remaining == 0) return null;
        if (_.data.len - _.pos < 50) {
            _.remaining = 0;
            return null;
        }

        const p = _.pos;
        const tri = StlTriangle {
            normal: readVec3LE(_.data, p),
            v1: readVec3LE(_.data, p + 12),
            v2: readVec3LE(_.data, p + 24),
            v3: readVec3LE(_.data, p + 36),
            attr_byte_count: readU16LE(_.data, p + 48),
        };
        _.pos = p + 50;
        _.remaining -= 1;
        return tri;
    }
}

/// 零分配编码完整二进制 STL 到目标缓冲区 `out`；缓冲区不足或超出 `u32` 上限时返回 `null`
pub fn writeStlBinary(triangles: []const StlTriangle, header_text: []const u8, out: []u8): ([]const u8)? {
    if (triangles.len > 0xFFFFFFFF) return null;
    if (out.len < 84) return null;
    const max_tris: usize = (out.len - 84) / 50;
    if (triangles.len > max_tris) return null;

    // 80-byte header padded with NUL
    let i: usize = 0;
    while (i < 80) {
        out[i] = if (i < header_text.len) header_text[i] else 0;
        i += 1;
    }

    // 4-byte triangle count
    writeU32LE(out, 80, triangles.len as u32);

    // 50-byte triangle records
    let pos: usize = 84;
    let idx: usize = 0;
    while (idx < triangles.len) {
        const t = triangles[idx];
        writeVec3LE(out, pos, t.normal);
        writeVec3LE(out, pos + 12, t.v1);
        writeVec3LE(out, pos + 24, t.v2);
        writeVec3LE(out, pos + 36, t.v3);
        writeU16LE(out, pos + 48, t.attr_byte_count);
        pos += 50;
        idx += 1;
    }

    return out[0..pos];
}

/// 零分配统计二进制 STL 网格的轴对齐包围盒（AABB）；空网格或非法头返回 `null`
pub fn computeStlBinaryBounds(data: []const u8): StlBounds? {
    let it = StlBinaryIter.init(data);
    const first = it.next() ?? return null;

    let min_x = first.v1.x;
    let min_y = first.v1.y;
    let min_z = first.v1.z;
    let max_x = first.v1.x;
    let max_y = first.v1.y;
    let max_z = first.v1.z;
    let count: usize = 1;

    // Include first.v2, first.v3
    if (first.v2.x < min_x) min_x = first.v2.x;
    if (first.v2.y < min_y) min_y = first.v2.y;
    if (first.v2.z < min_z) min_z = first.v2.z;
    if (first.v2.x > max_x) max_x = first.v2.x;
    if (first.v2.y > max_y) max_y = first.v2.y;
    if (first.v2.z > max_z) max_z = first.v2.z;

    if (first.v3.x < min_x) min_x = first.v3.x;
    if (first.v3.y < min_y) min_y = first.v3.y;
    if (first.v3.z < min_z) min_z = first.v3.z;
    if (first.v3.x > max_x) max_x = first.v3.x;
    if (first.v3.y > max_y) max_y = first.v3.y;
    if (first.v3.z > max_z) max_z = first.v3.z;

    while (tri := it.next()) {
        count += 1;
        const verts: [3]StlVec3 = [tri.v1, tri.v2, tri.v3];
        let k: usize = 0;
        while (k < 3) {
            const v = verts[k];
            if (v.x < min_x) min_x = v.x;
            if (v.y < min_y) min_y = v.y;
            if (v.z < min_z) min_z = v.z;
            if (v.x > max_x) max_x = v.x;
            if (v.y > max_y) max_y = v.y;
            if (v.z > max_z) max_z = v.z;
            k += 1;
        }
    }

    return StlBounds {
        min: StlVec3 { x: min_x, y: min_y, z: min_z },
        max: StlVec3 { x: max_x, y: max_y, z: max_z },
        triangle_count: count,
    };
}

/// 零分配统计 ASCII STL 中 `facet normal` 三角面片声明数量（大小写不敏感）
pub fn countAsciiStlFacets(data: []const u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    let prev_was_facet = false;

    while (i < data.len) {
        while (i < data.len && isSpace(data[i])) {
            i += 1;
        }
        if (i >= data.len) break;

        const start = i;
        while (i < data.len && !isSpace(data[i])) {
            i += 1;
        }
        const word = data[start..i];

        if (prev_was_facet) {
            if (strEqlIgnoreCase(word, "normal")) {
                count += 1;
                prev_was_facet = false;
                continue;
            }
        }
        prev_was_facet = strEqlIgnoreCase(word, "facet");
    }

    return count;
}
