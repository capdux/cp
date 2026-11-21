// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc LevelDB/RocksDB SSTable Parser)

/// LevelDB / RocksDB SSTable 固定 48 字节 Footer 大小
pub const SST_FOOTER_SIZE: usize = 48;

/// LevelDB SSTable Footer 8 字节小端魔数
pub const SST_LEVELDB_MAGIC: u64 = 0xDB4775248B80FB57;

/// RocksDB BlockBasedTable Footer 8 字节小端魔数
pub const SST_ROCKSDB_MAGIC: u64 = 0x88E241B785F4CFF7;

/// SSTable Block 尾部元数据大小（1B 压缩类型 + 4B masked CRC32C）
pub const SST_BLOCK_TRAILER_SIZE: usize = 5;

const SST_CRC_MASK_DELTA: u32 = 0xA282EAD8;

/// SSTable Block 压缩类型枚举
pub enum SstCompression: u8 {
    none = 0,
    snappy = 1,
    zlib = 2,
    bzip2 = 3,
    lz4 = 4,
    lz4hc = 5,
    zstd = 7,
    unknown = 255,
}

/// Varint64 解码结果（值与下一字节偏移）
pub struct VarintResult {
    value: u64,
    next_pos: usize,
}

/// SSTable Block 偏移与字节大小句柄
pub struct BlockHandle {
    offset: u64,
    size: u64,
}

/// BlockHandle 解码结果
pub struct BlockHandleResult {
    handle: BlockHandle,
    next_pos: usize,
}

/// 解析后的 48 字节 SSTable Footer
pub struct SstFooter {
    magic: u64,
    is_rocksdb: bool,
    metaindex_handle: BlockHandle,
    index_handle: BlockHandle,
}

/// 零拷贝 SSTable Block 视图（负载、压缩算法与 4B masked CRC32C）
pub struct SstBlockView {
    payload: []const u8,
    compression: SstCompression,
    masked_crc32c: u32,

    /// 返回解掩码后的原始 CRC32C 校验值
    fn crc32c(): u32 {
        return unmaskCrc(_.masked_crc32c);
    }
}

/// 前缀压缩 Data/Index Block 头部信息（重启点数量与偏移表）
pub struct SstBlockHeader {
    payload: []const u8,
    num_restarts: u32,
    restarts_offset: usize,

    /// 读取第 `idx` 个重启点在块内的字节偏移；越界返回 `null`
    fn restartOffset(idx: u32): u32? {
        if (idx >= _.num_restarts) return null;
        const off: usize = _.restarts_offset + ((idx as usize) * 4);
        return readU32LE(_.payload, off);
    }

    /// 创建零分配前缀压缩条目迭代器
    fn entries(): SstBlockIter {
        return SstBlockIter.init(_.payload, _.restarts_offset);
    }
}

/// 零拷贝前缀压缩 Block 单条原始记录
pub struct SstRawEntry {
    shared_bytes: u32,
    non_shared_bytes: u32,
    key_delta: []const u8,
    value: []const u8,
}

/// 零堆分配前缀压缩 Block 条目流式迭代器（严格不越过 `restarts_offset`）
pub struct SstBlockIter {
    payload: []const u8,
    pos: usize = 0,
    limit: usize = 0,

    static fn init(payload: []const u8, restarts_offset: usize): SstBlockIter {
        const clamped = if (restarts_offset <= payload.len) restarts_offset else payload.len;
        return SstBlockIter {
            payload: payload,
            pos: 0,
            limit: clamped,
        };
    }

    /// 读取下一条 `(shared, non_shared, key_delta, value)`；到达重启点表或遇到截断条目时安全返回 `null`
    fn next(*_): SstRawEntry? {
        if (_.pos >= _.limit) return null;

        const slice = _.payload[0.._.limit];
        const r_shared = decodeVarint64(slice, _.pos) ?? return null;
        const r_non_shared = decodeVarint64(slice, r_shared.next_pos) ?? return null;
        const r_val_len = decodeVarint64(slice, r_non_shared.next_pos) ?? return null;

        if (r_shared.value > 0xFFFFFFFF || r_non_shared.value > 0xFFFFFFFF || r_val_len.value > 0xFFFFFFFF) {
            _.pos = _.limit;
            return null;
        }

        const cur = r_val_len.next_pos;
        const non_shared: usize = r_non_shared.value as usize;
        const val_len: usize = r_val_len.value as usize;

        if (non_shared > _.limit - cur) {
            _.pos = _.limit;
            return null;
        }
        const val_start: usize = cur + non_shared;
        if (val_len > _.limit - val_start) {
            _.pos = _.limit;
            return null;
        }

        const key_delta = _.payload[cur..val_start];
        const value = _.payload[val_start .. val_start + val_len];
        _.pos = val_start + val_len;

        return SstRawEntry {
            shared_bytes: r_shared.value as u32,
            non_shared_bytes: r_non_shared.value as u32,
            key_delta: key_delta,
            value: value,
        };
    }
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

fn byteToCompression(b: u8): SstCompression {
    if (b == 0) return SstCompression.none;
    if (b == 1) return SstCompression.snappy;
    if (b == 2) return SstCompression.zlib;
    if (b == 3) return SstCompression.bzip2;
    if (b == 4) return SstCompression.lz4;
    if (b == 5) return SstCompression.lz4hc;
    if (b == 7) return SstCompression.zstd;
    return SstCompression.unknown;
}

/// LevelDB / RocksDB 标准 CRC32C 掩码函数
pub fn maskCrc(crc: u32): u32 {
    const rot: u32 = (crc >> 15) | (crc << 17);
    return (((rot as u64) + (SST_CRC_MASK_DELTA as u64)) & 0xFFFFFFFF) as u32;
}

/// LevelDB / RocksDB 标准 CRC32C 解掩码函数
pub fn unmaskCrc(masked: u32): u32 {
    const rot: u32 = (((masked as u64) + 0x100000000 - (SST_CRC_MASK_DELTA as u64)) & 0xFFFFFFFF) as u32;
    return (rot >> 17) | (rot << 15);
}

/// 解码无符号 LEB128 Varint64（最多 10 字节，严格防溢出与截断）
pub fn decodeVarint64(data: []const u8, pos: usize): VarintResult? {
    let acc: u64 = 0;
    let shift: u64 = 0;
    let i: usize = pos;

    while (i < data.len && shift <= 63) {
        const b = data[i];
        if (shift == 63 && b > 1) return null;
        acc |= ((b & 0x7F) as u64) << (shift as u6);
        i += 1;
        if ((b & 0x80) == 0) {
            return VarintResult {
                value: acc,
                next_pos: i,
            };
        }
        shift += 7;
    }
    return null;
}

/// 编码无符号 LEB128 Varint64 到目标切片，返回写入字节数；空间不足返回 `null`
pub fn encodeVarint64(buf: *[]u8, v: u64): usize? {
    let x = v;
    let i: usize = 0;
    while (true) {
        if (i >= buf.len) return null;
        if (x < 0x80) {
            buf[i] = x as u8;
            return i + 1;
        }
        buf[i] = ((x & 0x7F) as u8) | 0x80;
        x = x >> 7;
        i += 1;
    }
}

/// 解码 `BlockHandle`（连续两个 Varint64：`offset` 与 `size`）
pub fn decodeBlockHandle(data: []const u8, pos: usize): BlockHandleResult? {
    const r_off = decodeVarint64(data, pos) ?? return null;
    const r_sz = decodeVarint64(data, r_off.next_pos) ?? return null;
    return BlockHandleResult {
        handle: BlockHandle {
            offset: r_off.value,
            size: r_sz.value,
        },
        next_pos: r_sz.next_pos,
    };
}

/// 编码 `BlockHandle` 到目标切片，返回总写入字节数；空间不足返回 `null`
pub fn encodeBlockHandle(buf: *[]u8, h: BlockHandle): usize? {
    const n1 = encodeVarint64(buf, h.offset) ?? return null;
    let tail = buf[n1..buf.len];
    const n2 = encodeVarint64(*tail, h.size) ?? return null;
    return n1 + n2;
}

fn validateHandleBounds(file_len: usize, h: BlockHandle): bool {
    const total: u64 = file_len as u64;
    if (h.offset > total) return false;
    const rem: u64 = total - h.offset;
    if (rem < (SST_BLOCK_TRAILER_SIZE as u64)) return false;
    return h.size <= rem - (SST_BLOCK_TRAILER_SIZE as u64);
}

/// 解析文件末尾 48 字节 LevelDB / RocksDB SSTable Footer 并校验块句柄边界
pub fn parseSstFooter(data: []const u8): SstFooter? {
    if (data.len < SST_FOOTER_SIZE) return null;

    const foot_off: usize = data.len - SST_FOOTER_SIZE;
    const foot = data[foot_off..data.len];
    const magic = readU64LE(foot, 40);

    let is_rocksdb: bool = false;
    if (magic == SST_LEVELDB_MAGIC) {
        is_rocksdb = false;
    } else if (magic == SST_ROCKSDB_MAGIC) {
        is_rocksdb = true;
    } else {
        return null;
    }

    const r_meta = decodeBlockHandle(foot[0..40], 0) ?? return null;
    const r_index = decodeBlockHandle(foot[0..40], r_meta.next_pos) ?? return null;

    if (!validateHandleBounds(data.len, r_meta.handle)) return null;
    if (!validateHandleBounds(data.len, r_index.handle)) return null;

    return SstFooter {
        magic: magic,
        is_rocksdb: is_rocksdb,
        metaindex_handle: r_meta.handle,
        index_handle: r_index.handle,
    };
}

/// 探测缓冲区末尾是否包含合法的 LevelDB / RocksDB SSTable Footer
pub fn isSst(data: []const u8): bool {
    return parseSstFooter(data) != null;
}

/// 依据 `BlockHandle` 从 SSTable 文件缓冲区零拷贝提取块负载、压缩类型与 masked CRC32C
pub fn extractSstBlock(file_data: []const u8, handle: BlockHandle): SstBlockView? {
    if (!validateHandleBounds(file_data.len, handle)) return null;

    const off: usize = handle.offset as usize;
    const sz: usize = handle.size as usize;
    const comp_off: usize = off + sz;

    return SstBlockView {
        payload: file_data[off..comp_off],
        compression: byteToCompression(file_data[comp_off]),
        masked_crc32c: readU32LE(file_data, comp_off + 1),
    };
}

/// 解析未压缩 Data/Index Block 尾部的重启点表，校验边界并返回 `SstBlockHeader`
pub fn parseSstBlockInfo(block_payload: []const u8): SstBlockHeader? {
    if (block_payload.len < 4) return null;

    const num_restarts = readU32LE(block_payload, block_payload.len - 4);
    if (num_restarts == 0) return null;

    const trailer_bytes: u64 = ((num_restarts as u64) + 1) * 4;
    if (trailer_bytes > (block_payload.len as u64)) return null;

    const restarts_offset: usize = block_payload.len - (trailer_bytes as usize);
    return SstBlockHeader {
        payload: block_payload,
        num_restarts: num_restarts,
        restarts_offset: restarts_offset,
    };
}
