// Origin: Pure-CP Stdlib (Zero-Alloc glTF 2.0 Binary / GLB Container Parser) | Created: 2026-11-22

/// glTF 2.0 Binary (GLB) 魔数与版本常量
pub const GLB_MAGIC: u32 = 0x46546C67; // "glTF"
pub const GLB_VERSION_2: u32 = 2;

/// GLB Chunk 原始类型常量（Little-Endian ASCII）
pub const CHUNK_TYPE_JSON: u32 = 0x4E4F534A; // "JSON"
pub const CHUNK_TYPE_BIN: u32 = 0x004E4942;  // "BIN\0"

/// glTF 2.0 Accessor ComponentType 常量（§3.6.2.2）
pub const COMPONENT_BYTE: u32 = 5120;
pub const COMPONENT_UNSIGNED_BYTE: u32 = 5121;
pub const COMPONENT_SHORT: u32 = 5122;
pub const COMPONENT_UNSIGNED_SHORT: u32 = 5123;
pub const COMPONENT_UNSIGNED_INT: u32 = 5125;
pub const COMPONENT_FLOAT: u32 = 5126;

/// GLB Chunk 分类枚举
pub enum ChunkType: u8 {
    json = 0,
    bin = 1,
    unknown = 2,
}

/// glTF 2.0 Accessor 元素类型枚举
pub enum AccessorType: u8 {
    scalar = 0,
    vec2 = 1,
    vec3 = 2,
    vec4 = 3,
    mat2 = 4,
    mat3 = 5,
    mat4 = 6,
}

/// 12 字节 GLB 文件头元数据
pub struct GlbHeader {
    magic: u32,
    version: u32,
    length: u32,
}

/// 零拷贝 GLB Chunk 视图
pub struct GlbChunk {
    chunk_type: ChunkType,
    raw_type: u32,
    length: u32,
    data: []const u8,
}

/// 完整校验后的零拷贝 GLB 文档视图
pub struct GlbDocument {
    header: GlbHeader,
    json: []const u8,
    bin: ([]const u8)?,
    chunk_count: usize,
}

fn readU32LE(data: []const u8, off: usize): u32 {
    return (data[off] as u32)
        | ((data[off + 1] as u32) << 8)
        | ((data[off + 2] as u32) << 16)
        | ((data[off + 3] as u32) << 24);
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

fn classifyChunkType(raw_type: u32): ChunkType {
    if (raw_type == CHUNK_TYPE_JSON) return ChunkType.json;
    if (raw_type == CHUNK_TYPE_BIN) return ChunkType.bin;
    return ChunkType.unknown;
}

/// 快速探测缓冲区是否以合法 glTF 2.0 GLB 头部开头
pub fn isGlb(data: []const u8): bool {
    return parseGlbHeader(data) != null;
}

/// 解析并校验 12 字节 GLB 头部（`magic == 0x46546C67`、`version == 2`、总长度 4 字节对齐且不越界）
pub fn parseGlbHeader(data: []const u8): GlbHeader? {
    if (data.len < 12) return null;
    const magic = readU32LE(data, 0);
    if (magic != GLB_MAGIC) return null;

    const version = readU32LE(data, 4);
    if (version != GLB_VERSION_2) return null;

    const length = readU32LE(data, 8);
    if (length < 12 || (length & 3) != 0) return null;
    if ((length as usize) > data.len) return null;

    return GlbHeader {
        magic: magic,
        version: version,
        length: length,
    };
}

/// 零堆分配 GLB Chunk 流式迭代器（严格执行 4 字节对齐与边界检查）
pub struct GlbChunkIter {
    data: []const u8,
    pos: usize = 12,
    limit: usize = 0,
    had_error: bool = false,

    pub static fn init(data: []const u8): GlbChunkIter {
        if (hdr := parseGlbHeader(data)) {
            return GlbChunkIter {
                data: data,
                pos: 12,
                limit: hdr.length as usize,
                had_error: false,
            };
        }
        return GlbChunkIter {
            data: data,
            pos: 0,
            limit: 0,
            had_error: true,
        };
    }

    /// 读取下一个 GLB Chunk；到达末尾返回 `null`，遇到畸形截断或非 4 字节对齐时置 `had_error = true` 并返回 `null`
    pub fn next(*_): GlbChunk? {
        if (_.had_error || _.pos >= _.limit) return null;
        if (_.limit - _.pos < 8) {
            _.had_error = true;
            _.pos = _.limit;
            return null;
        }

        const chunk_len = readU32LE(_.data, _.pos);
        const raw_type = readU32LE(_.data, _.pos + 4);
        const len_usize = chunk_len as usize;

        // glTF 2.0 spec: every chunk payload length must be 4-byte aligned and within total length
        if ((chunk_len & 3) != 0 || len_usize > _.limit - (_.pos + 8)) {
            _.had_error = true;
            _.pos = _.limit;
            return null;
        }

        const payload = _.data[_.pos + 8 .. _.pos + 8 + len_usize];
        _.pos += 8 + len_usize;

        return GlbChunk {
            chunk_type: classifyChunkType(raw_type),
            raw_type: raw_type,
            length: chunk_len,
            data: payload,
        };
    }
}

/// 快捷提取 GLB 首块 JSON Chunk 负载切片
pub fn extractJsonChunk(data: []const u8): ([]const u8)? {
    let it = GlbChunkIter.init(data);
    const first = it.next() ?? return null;
    if (first.chunk_type != ChunkType.json) return null;
    return first.data;
}

/// 快捷提取 GLB 中的首个 BIN Chunk 负载切片（安全跳过未知扩展 Chunk）
pub fn extractBinChunk(data: []const u8): ([]const u8)? {
    let it = GlbChunkIter.init(data);
    while (chunk := it.next()) {
        if (chunk.chunk_type == ChunkType.bin) {
            return chunk.data;
        }
    }
    return null;
}

/// 完整解析并校验 GLB 2.0 容器（首块必须为 JSON，提取可选 BIN 并安全跳过未知 Chunk，拒绝截断/非对齐畸形包）
pub fn parseGlb(data: []const u8): GlbDocument? {
    const hdr = parseGlbHeader(data) ?? return null;
    let it = GlbChunkIter.init(data);

    const first = it.next() ?? return null;
    if (first.chunk_type != ChunkType.json) return null;

    let bin_slice: ([]const u8)? = null;
    let count: usize = 1;

    while (chunk := it.next()) {
        count += 1;
        if (chunk.chunk_type == ChunkType.bin && bin_slice == null) {
            bin_slice = chunk.data;
        }
    }

    if (it.had_error) return null;

    return GlbDocument {
        header: hdr,
        json: first.data,
        bin: bin_slice,
        chunk_count: count,
    };
}

/// 查询 glTF 2.0 ComponentType 单分量字节宽度（`BYTE/UBYTE=1`, `SHORT/USHORT=2`, `UINT/FLOAT=4`）
pub fn componentByteSize(component_type: u32): usize? {
    if (component_type == COMPONENT_BYTE || component_type == COMPONENT_UNSIGNED_BYTE) return 1;
    if (component_type == COMPONENT_SHORT || component_type == COMPONENT_UNSIGNED_SHORT) return 2;
    if (component_type == COMPONENT_UNSIGNED_INT || component_type == COMPONENT_FLOAT) return 4;
    return null;
}

/// 查询 glTF 2.0 AccessorType 包含的标量分量个数（`SCALAR=1`..`MAT4=16`）
pub fn accessorComponentCount(accessor_type: AccessorType): usize {
    if (accessor_type == AccessorType.scalar) return 1;
    if (accessor_type == AccessorType.vec2) return 2;
    if (accessor_type == AccessorType.vec3) return 3;
    if (accessor_type == AccessorType.vec4) return 4;
    if (accessor_type == AccessorType.mat2) return 4;
    if (accessor_type == AccessorType.mat3) return 9;
    return 16;
}

/// 从 JSON 字符串名称解析 `AccessorType`（`"SCALAR"` / `"VEC2"`..`"MAT4"`）
pub fn parseAccessorType(name: []const u8): AccessorType? {
    if (strEq(name, "SCALAR")) return AccessorType.scalar;
    if (strEq(name, "VEC2")) return AccessorType.vec2;
    if (strEq(name, "VEC3")) return AccessorType.vec3;
    if (strEq(name, "VEC4")) return AccessorType.vec4;
    if (strEq(name, "MAT2")) return AccessorType.mat2;
    if (strEq(name, "MAT3")) return AccessorType.mat3;
    if (strEq(name, "MAT4")) return AccessorType.mat4;
    return null;
}

/// 计算指定 `componentType` 与 `AccessorType` 的单元素紧凑字节步长（如 `FLOAT + VEC3 = 12`）
pub fn accessorElementStride(component_type: u32, accessor_type: AccessorType): usize? {
    const comp_size = componentByteSize(component_type) ?? return null;
    return comp_size * accessorComponentCount(accessor_type);
}
