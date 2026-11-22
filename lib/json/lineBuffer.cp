// Source: Absorbed from /data/dev/bun/src/jsc/JSONLineBuffer.cp | Synced: 2026-11-22T21:26:39Z
// Pure-CP zero-allocation O(n) single-pass newline-delimited (JSONL / NDJSON) streaming buffer.
// Tracks `head`, `scanned_pos`, and `newline_pos` so every byte is scanned at most once,
// advancing `head` in O(1) on consume and compacting lazily when `head` reaches threshold.

/// Bun 默认压缩阈值常量（16 MiB）
pub const JSON_LINE_DEFAULT_COMPACTION_THRESHOLD: u32 = 16 * 1024 * 1024;

/// `JsonLineBuffer` 容量与写回校验错误集
pub err JsonLineBufferError {
    BufferOverflow,
    WrittenExceedsCapacity,
}

/// 单条完整 JSONL/NDJSON 行视图（零拷贝切片，包含末尾 `\n`）
pub struct JsonLineNext {
    /// 包含终止 `\n` 在内的完整行字节切片（有效期直至下一次修改缓冲区）
    data: []const u8,
    /// 终止 `\n` 相对当前 `head` 的字节偏移（等于 `data.len - 1`）
    newline_pos: u32,

    /// 零拷贝去除末尾 `\n`（以及可选的 `\r\n` 回车符），返回纯 JSON 载荷切片
    fn payload(): []const u8 {
        let end: usize = @as(usize, _.newline_pos);
        if (end > _.data.len) {
            end = _.data.len;
        }
        if (end > 0 && _.data[end - 1] == '\r') {
            end -= 1;
        }
        return _.data[0..end];
    }

    /// 本条完整行需要传给 `consume()` 的总字节数（`newline_pos + 1`）
    fn consumedBytes(): u32 {
        return _.newline_pos +| 1;
    }
}

/// 若输入字节切片以 UTF-8 BOM (`0xEF 0xBB 0xBF`) 开头则零拷贝剥离，否则原样返回。
pub fn stripUtf8Bom(bytes: []const u8): []const u8 {
    if (bytes.len >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        return bytes[3..];
    }
    return bytes;
}

/// 零堆分配单遍扫描换行分隔流缓冲区（对应 Bun `JSONLineBuffer`）。
///
/// 核心不变式：
/// - `head <= len <= storage.len`
/// - `scanned_pos <= len - head`
/// - 若 `newline_pos` 非空，则 `activeSlice()[newline_pos] == '\n'` 且 `newline_pos < scanned_pos`
/// - 每个入队字节至多被扫描一次，彻底消除增量分片流中的 O(n^2) 重复扫描。
pub struct JsonLineBuffer {
    storage: []u8,
    len: u32 = 0,
    head: u32 = 0,
    newline_pos: u32? = null,
    scanned_pos: u32 = 0,
    compaction_threshold: u32 = 0,
    total_bytes_scanned: u64 = 0,
    compaction_count: u64 = 0,

    /// 使用调用方提供的固定或外部切片初始化零堆分配行缓冲区。
    /// 默认压缩阈值为缓冲区容量的一半（至少为 1）。
    static fn init(buf: []u8): JsonLineBuffer {
        const half: usize = buf.len / 2;
        const thresh: u32 = if (half == 0) {
            1
        } else if (half > 0xFFFFFFFF) {
            0xFFFFFFFF
        } else {
            @as(u32, @intCast(half))
        };
        return JsonLineBuffer{
            storage: buf,
            compaction_threshold: thresh,
        };
    }

    /// 使用自定义 `head` 压缩阈值初始化行缓冲区（阈值为 0 时按 1 处理）。
    static fn initWithThreshold(buf: []u8, threshold: u32): JsonLineBuffer {
        const effective: u32 = if (threshold == 0) 1 else threshold;
        return JsonLineBuffer{
            storage: buf,
            compaction_threshold: effective,
        };
    }

    /// 返回当前未消费的活跃数据切片（零拷贝）
    fn activeSlice(): []const u8 {
        const h = @as(usize, _.head);
        const l = @as(usize, _.len);
        if (h >= l || l > _.storage.len) {
            return _.storage[0..0];
        }
        return _.storage[h..l];
    }

    /// 当前未消费字节数
    fn activeLen(): usize {
        if (_.head >= _.len) {
            return 0;
        }
        return @as(usize, _.len - _.head);
    }

    /// 底层缓冲区总容量（字节）
    fn capacity(): usize {
        return _.storage.len;
    }

    /// 尾部尚未使用的连续可写容量（未压缩前）
    fn unusedCapacity(): usize {
        const l = @as(usize, _.len);
        if (l >= _.storage.len) {
            return 0;
        }
        return _.storage.len - l;
    }

    /// 总可用写入空间（包含压缩后可回收的 `head` 前缀空间）
    fn availableSpace(): usize {
        const active = _.activeLen();
        if (active >= _.storage.len) {
            return 0;
        }
        return _.storage.len - active;
    }

    /// 当前缓冲区是否已全部消费为空
    fn isEmpty(): bool {
        return _.head >= _.len;
    }

    /// 是否已经定位到至少一条完整换行分隔记录
    fn hasCompleteLine(): bool {
        return _.newline_pos != null;
    }

    /// 返回尾部连续可写切片，供 socket / pipe 零拷贝直接读入，完成后调用 `notifyWritten()`。
    fn unusedCapacitySlice(*_): []u8 {
        const l = @as(usize, _.len);
        if (l >= _.storage.len) {
            return _.storage[_.storage.len.._.storage.len];
        }
        return _.storage[l..];
    }

    /// 仅扫描 `activeSlice()[scanned_pos..]` 中尚未检查的新增字节，确保单遍 O(n) 扫描。
    fn scanForNewline(*_) {
        if (_.newline_pos != null) {
            return;
        }
        const slice = _.activeSlice();
        const start = @as(usize, _.scanned_pos);
        if (start >= slice.len) {
            return;
        }

        let i: usize = start;
        while (i < slice.len) {
            _.total_bytes_scanned += 1;
            if (slice[i] == '\n') {
                const pos = @as(u32, @intCast(i));
                _.newline_pos = pos;
                _.scanned_pos = pos +| 1;
                return;
            }
            i += 1;
        }

        _.scanned_pos = @as(u32, @intCast(slice.len));
    }

    /// 将未消费数据前移至缓冲区起点以回收 `head` 前缀空间。
    /// 若发生搬移返回 `true`，若 `head == 0` 返回 `false`。
    fn compact(*_): bool {
        if (_.head == 0) {
            return false;
        }
        const h = @as(usize, _.head);
        const l = @as(usize, _.len);
        if (h >= l) {
            _.head = 0;
            _.len = 0;
            _.scanned_pos = 0;
            _.newline_pos = null;
            _.compaction_count += 1;
            return true;
        }
        const active_len = l - h;
        let i: usize = 0;
        while (i < active_len) {
            _.storage[i] = _.storage[h + i];
            i += 1;
        }
        _.len = @as(u32, @intCast(active_len));
        _.head = 0;
        _.compaction_count += 1;
        return true;
    }

    /// 追加新到达的字节切片，仅对新增字节扫描换行符。
    /// 若尾部空间不足但回收 `head` 后可容纳，则自动触发 `compact()`；否则返回 `BufferOverflow`。
    fn append(*_, bytes: []const u8): JsonLineBufferError!usize {
        if (bytes.len == 0) {
            return _.activeLen();
        }
        if (bytes.len > _.unusedCapacity()) {
            if (bytes.len > _.availableSpace()) {
                return JsonLineBufferError.BufferOverflow;
            }
            * = _.compact();
        }
        const dst_start = @as(usize, _.len);
        let i: usize = 0;
        while (i < bytes.len) {
            _.storage[dst_start + i] = bytes[i];
            i += 1;
        }
        _.len = @as(u32, @intCast(dst_start + bytes.len));
        _.scanForNewline();
        return _.activeLen();
    }

    /// 通知缓冲区外部已直接向 `unusedCapacitySlice()` 写入 `written_len` 字节并增量扫描换行符。
    fn notifyWritten(*_, written_len: usize): JsonLineBufferError!usize {
        if (written_len > _.unusedCapacity()) {
            return JsonLineBufferError.WrittenExceedsCapacity;
        }
        if (written_len == 0) {
            return _.activeLen();
        }
        _.len = @as(u32, @intCast(@as(usize, _.len) + written_len));
        _.scanForNewline();
        return _.activeLen();
    }

    /// 查看下一条完整行（包含末尾 `\n`）；若当前尚无完整行则返回 `null`。
    /// 注意：请在消费完返回的切片内容后再调用 `consume(res.consumedBytes())`。
    fn next(): JsonLineNext? {
        if (pos := _.newline_pos) {
            const slice = _.activeSlice();
            const end = @as(usize, pos) + 1;
            if (end > slice.len) {
                return null;
            }
            return JsonLineNext{
                data: slice[0..end],
                newline_pos: pos,
            };
        }
        return null;
    }

    /// 从缓冲区前端消费 `bytes` 字节：O(1) 推进 `head`，仅在跨过已知 `\n` 时扫描下一条，
    /// 全部消费完毕自动归零，或 `head >= compaction_threshold` 时懒压缩。
    fn consume(*_, bytes: u32) {
        if (bytes == 0) {
            return;
        }
        const next_head = _.head +| bytes;
        if (next_head >= _.len) {
            _.head = 0;
            _.len = 0;
            _.scanned_pos = 0;
            _.newline_pos = null;
            return;
        }
        _.head = next_head;

        if (bytes >= _.scanned_pos) {
            _.scanned_pos = 0;
        } else {
            _.scanned_pos -= bytes;
        }

        if (pos := _.newline_pos) {
            if (bytes > pos) {
                _.newline_pos = null;
                _.scanForNewline();
            } else {
                _.newline_pos = pos - bytes;
            }
        }

        if (_.head >= _.compaction_threshold) {
            * = _.compact();
        }
    }

    /// 跳过并消费当前第一条完整行（若存在），返回是否成功跳过。
    fn skipLine(*_): bool {
        if (pos := _.newline_pos) {
            _.consume(pos +| 1);
            return true;
        }
        return false;
    }

    /// 清空当前活跃数据并重置扫描游标（保留底层切片与压缩阈值）。
    fn reset(*_) {
        _.len = 0;
        _.head = 0;
        _.scanned_pos = 0;
        _.newline_pos = null;
    }
}
