// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc JVM Java Flight Recorder v2.0 Parser)

/// JVM JFR v2.0 常量与特性标志位
pub const JFR_MAGIC: []const u8 = "FLR\x00";
pub const JFR_CHUNK_HEADER_SIZE: usize = 68;
pub const JFR_EVENT_METADATA: u64 = 0;
pub const JFR_EVENT_CHECKPOINT: u64 = 1;
pub const JFR_FLAG_COMPRESSED_INTS: u32 = 0x01;
pub const JFR_FLAG_FINAL_CHUNK: u32 = 0x02;

/// 68 字节大端 JFR Chunk 头解析结构
pub struct JfrChunkHeader {
    major_version: u16,
    minor_version: u16,
    chunk_size: u64,
    constant_pool_offset: u64,
    metadata_offset: u64,
    start_time_nanos: i64,
    duration_nanos: i64,
    start_ticks: i64,
    ticks_per_second: i64,
    features: u32,

    /// 是否启用压缩整数（LEB128）编码
    fn usesCompressedInts(): bool {
        return (_.features & JFR_FLAG_COMPRESSED_INTS) != 0;
    }

    /// 是否为录制流最后一个 Chunk
    fn isFinalChunk(): bool {
        return (_.features & JFR_FLAG_FINAL_CHUNK) != 0;
    }

    /// 返回 Chunk 持续时间（毫秒）
    fn durationMs(): f64 {
        return (_.duration_nanos as f64) / 1000000.0;
    }
}

/// 多 Chunk 迭代产出的单个 Chunk 视图
pub struct JfrChunk {
    offset: usize,
    header: JfrChunkHeader,
    slice: []const u8,
}

/// JFR 压缩无符号整数（最多 9 字节）解码结果
pub struct JfrVarint {
    value: u64,
    next_pos: usize,
}

/// JFR 单条事件帧零拷贝视图
pub struct JfrEventFrame {
    size: u64,
    event_type_id: u64,
    payload: []const u8,
}

/// JFR 单遍扫描汇总统计
pub struct JfrStats {
    chunk_count: u64,
    event_count: u64,
    metadata_count: u64,
    checkpoint_count: u64,
    total_duration_ms: f64,
}

fn readU16BE(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
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

fn u64ToI64(u: u64): i64 {
    if (u >= 0x8000000000000000) {
        const inv = 0xFFFFFFFFFFFFFFFF - u;
        return -(inv as i64) - 1;
    }
    return u as i64;
}

fn readI64BE(data: []const u8, off: usize): i64 {
    return u64ToI64(readU64BE(data, off));
}

/// 解析位于 `offset` 处的 68 字节大端 JFR Chunk 头
pub fn parseJfrChunkHeader(data: []const u8, offset: usize): JfrChunkHeader? {
    if (offset > data.len || data.len - offset < JFR_CHUNK_HEADER_SIZE) return null;
    if (data[offset] != 'F' || data[offset + 1] != 'L' || data[offset + 2] != 'R' || data[offset + 3] != 0) {
        return null;
    }

    const major = readU16BE(data, offset + 4);
    if (major < 1) return null;
    const minor = readU16BE(data, offset + 6);

    const chunk_size = readU64BE(data, offset + 8);
    const remain: u64 = (data.len - offset) as u64;
    if (chunk_size < (JFR_CHUNK_HEADER_SIZE as u64) || chunk_size > remain) return null;

    return JfrChunkHeader {
        major_version: major,
        minor_version: minor,
        chunk_size: chunk_size,
        constant_pool_offset: readU64BE(data, offset + 16),
        metadata_offset: readU64BE(data, offset + 24),
        start_time_nanos: readI64BE(data, offset + 32),
        duration_nanos: readI64BE(data, offset + 40),
        start_ticks: readI64BE(data, offset + 48),
        ticks_per_second: readI64BE(data, offset + 56),
        features: readU32BE(data, offset + 64),
    };
}

/// 校验字节切片是否以合法 JFR Chunk 头开头
pub fn isJfr(data: []const u8): bool {
    return parseJfrChunkHeader(data, 0) != null;
}

/// 解码 JFR 变长无符号整数（前 8 字节低 7 位有效、第 9 字节全 8 位有效，完美覆盖完整 `u64`）
pub fn readJfrUleb128(data: []const u8, pos: usize): JfrVarint? {
    if (pos >= data.len) return null;

    let acc: u64 = 0;
    let i: usize = 0;
    while (i < 8) {
        if (pos + i >= data.len) return null;
        const b = data[pos + i];
        const shift: u6 = (i * 7) as u6;
        acc |= ((b & 0x7F) as u64) << shift;
        if ((b & 0x80) == 0) {
            return JfrVarint { value: acc, next_pos: pos + i + 1 };
        }
        i += 1;
    }

    // 第 9 字节全 8 位均为有效数据位（bits 56..63）
    if (pos + 8 >= data.len) return null;
    const b8 = data[pos + 8];
    acc |= (b8 as u64) << 56;
    return JfrVarint { value: acc, next_pos: pos + 9 };
}

/// 零堆分配多 Chunk 拼接 `.jfr` 迭代器
pub struct JfrChunkIter {
    data: []const u8,
    pos: usize = 0,
    had_error: bool = false,

    static fn init(data: []const u8): JfrChunkIter {
        return JfrChunkIter {
            data: data,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): JfrChunk? {
        if (_.had_error || _.pos >= _.data.len) return null;
        if (hdr := parseJfrChunkHeader(_.data, _.pos)) {
            const off = _.pos;
            const len_usize = hdr.chunk_size as usize;
            const slice = _.data[off .. off + len_usize];
            _.pos = off + len_usize;
            return JfrChunk {
                offset: off,
                header: hdr,
                slice: slice,
            };
        }
        _.had_error = true;
        _.pos = _.data.len;
        return null;
    }
}

/// 零堆分配单个 Chunk 内事件帧迭代器（从偏移 68 开始按 `[varint size, varint type_id, payload]` 迭代）
pub struct JfrEventIter {
    data: []const u8,
    pos: usize,
    had_error: bool = false,

    static fn init(chunk_slice: []const u8): JfrEventIter {
        if (chunk_slice.len < JFR_CHUNK_HEADER_SIZE) {
            return JfrEventIter {
                data: chunk_slice,
                pos: chunk_slice.len,
                had_error: true,
            };
        }
        return JfrEventIter {
            data: chunk_slice,
            pos: JFR_CHUNK_HEADER_SIZE,
            had_error: false,
        };
    }

    fn next(*_): JfrEventFrame? {
        if (_.had_error || _.pos >= _.data.len) return null;
        const frame_start = _.pos;

        let sz = JfrVarint { value: 0, next_pos: 0 };
        if (v := readJfrUleb128(_.data, frame_start)) {
            sz = v;
        } else {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        if (sz.value == 0 || sz.value > ((_.data.len - frame_start) as u64)) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const frame_end = frame_start + (sz.value as usize);
        let tid = JfrVarint { value: 0, next_pos: 0 };
        if (v := readJfrUleb128(_.data, sz.next_pos)) {
            tid = v;
        } else {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        if (tid.next_pos > frame_end) {
            _.had_error = true;
            _.pos = _.data.len;
            return null;
        }

        const payload = _.data[tid.next_pos .. frame_end];
        _.pos = frame_end;

        return JfrEventFrame {
            size: sz.value,
            event_type_id: tid.value,
            payload: payload,
        };
    }
}

/// 单遍统计 `.jfr` 的 Chunk 数量、事件总数、Metadata/Checkpoint 事件数与总持续毫秒数
pub fn summarizeJfr(data: []const u8): JfrStats? {
    if (data.len < JFR_CHUNK_HEADER_SIZE) return null;
    let cit = JfrChunkIter.init(data);
    let stats = JfrStats {
        chunk_count: 0,
        event_count: 0,
        metadata_count: 0,
        checkpoint_count: 0,
        total_duration_ms: 0.0,
    };

    while (chunk := cit.next()) {
        stats.chunk_count += 1;
        stats.total_duration_ms += chunk.header.durationMs();

        let eit = JfrEventIter.init(chunk.slice);
        while (ev := eit.next()) {
            stats.event_count += 1;
            if (ev.event_type_id == JFR_EVENT_METADATA) {
                stats.metadata_count += 1;
            } else if (ev.event_type_id == JFR_EVENT_CHECKPOINT) {
                stats.checkpoint_count += 1;
            }
        }
        if (eit.had_error) return null;
    }

    if (cit.had_error || stats.chunk_count == 0) return null;
    return stats;
}
