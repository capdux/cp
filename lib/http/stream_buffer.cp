// Source: Absorbed from /data/dev/bun/src/http/ThreadSafeStreamBuffer.cp | Synced: 2026-11-22T11:12:13Z
// ============================================================================
// HTTP Thread-Safe Stream Buffer & Backpressure Control
// Absorbed from Bun: src/http/ThreadSafeStreamBuffer.cp & src/io/PipeWriter.cp
// ============================================================================

/// 默认缓冲区收缩阈值（4 KiB，对齐单页大小，消费完毕后若容量超过此值则收缩释放）
pub const DEFAULT_STREAM_SHRINK_THRESHOLD: usize = 4096;

/// `StreamBuffer` 与 `ThreadSafeStreamBuffer` 操作错误集
pub err StreamBufferError {
    OutOfMemory,
    BufferOverflow,
}

/// 排空通知回调（类型擦除，纯安全 CP 实现）
pub struct StreamDrainCallback {
    callback: fn(usize),
    context: usize,

    static fn init(cb: fn(usize), ctx: usize): StreamDrainCallback {
        return StreamDrainCallback{
            callback: cb,
            context: ctx,
        };
    }

    fn call() {
        const cb = _.callback;
        cb(_.context);
    }
}

/// 高性能游标式字节流缓冲区（零拷贝读切片 + 自动压缩 + 阈值收缩 + 可选背压上限）
pub struct StreamBuffer {
    allocator: std.mem.Allocator,
    list: std.ArrayListUnmanaged(u8) = .{},
    cursor: usize = 0,
    shrink_threshold: usize = DEFAULT_STREAM_SHRINK_THRESHOLD,
    max_bytes: usize = 0,

    /// 创建无上限的流缓冲区
    static fn init(allocator: std.mem.Allocator): StreamBuffer {
        return StreamBuffer{
            allocator: allocator,
        };
    }

    /// 创建带最大容量上限防护的流缓冲区（`max_bytes == 0` 表示无上限）
    static fn initBounded(allocator: std.mem.Allocator, max_bytes: usize): StreamBuffer {
        return StreamBuffer{
            allocator: allocator,
            max_bytes: max_bytes,
        };
    }

    /// RAII 确定性析构函数
    ~init() {
        _.list.deinit(_.allocator);
        _.cursor = 0;
    }

    /// 显式释放底层内存（幂等，可安全重复调用）
    fn deinit(*_) {
        _.list.deinit(_.allocator);
        _.list = .{};
        _.cursor = 0;
    }

    /// 返回当前未消费的字节数
    fn size(): usize {
        if (_.cursor >= _.list.items.len) {
            return 0;
        }
        return _.list.items.len - _.cursor;
    }

    /// 判断缓冲区是否为空
    fn isEmpty(): bool {
        return _.size() == 0;
    }

    /// 判断缓冲区是否非空
    fn isNotEmpty(): bool {
        return _.size() > 0;
    }

    /// 返回底层已分配容量（内存占用评估）
    fn memoryCost(): usize {
        return _.list.capacity;
    }

    /// 返回当前未消费数据的零拷贝只读切片
    fn slice(): []const u8 {
        if (_.cursor >= _.list.items.len) {
            return _.list.items[_.list.items.len..];
        }
        return _.list.items[_.cursor..];
    }

    /// 判断当前未消费字节数是否达到或超过高水位线（背压判定）
    fn isHighWater(high_water_mark: usize): bool {
        if (high_water_mark == 0) {
            return false;
        }
        return _.size() >= high_water_mark;
    }

    /// 预留至少 `additional` 字节的空闲写入容量
    fn ensureUnusedCapacity(*_, additional: usize): ! {
        if (additional == 0) {
            return;
        }
        const cur_size = _.size();
        if (_.max_bytes > 0) {
            if (additional > _.max_bytes || cur_size > _.max_bytes - additional) {
                return err.BufferOverflow;
            }
        }
        _.list.ensureUnusedCapacity(_.allocator, additional) catch {
            return err.OutOfMemory;
        };
    }

    /// 将未消费数据前移至索引 0，回收头部已消费空洞
    fn compact(*_) {
        if (_.cursor == 0) {
            return;
        }
        const remaining = _.size();
        if (remaining == 0) {
            _.list.clearRetainingCapacity();
            _.cursor = 0;
            return;
        }
        let i: usize = 0;
        while (i < remaining) {
            _.list.items[i] = _.list.items[_.cursor + i];
            i += 1;
        }
        _.list.items.len = remaining;
        _.cursor = 0;
    }

    /// 写入字节切片；若配置了 `max_bytes` 且超出上限则返回 `err.BufferOverflow`
    fn write(*_, data: []const u8): ! {
        if (data.len == 0) {
            return;
        }
        const cur_size = _.size();
        if (_.max_bytes > 0) {
            if (data.len > _.max_bytes || cur_size > _.max_bytes - data.len) {
                return err.BufferOverflow;
            }
        }
        // 当头部已消费空洞 >= 已写入长度的一半且达到 1024 字节时自动压缩，防止游标无限漂移
        if (_.cursor >= 1024 && _.cursor * 2 >= _.list.items.len) {
            _.compact();
        }
        _.list.appendSlice(_.allocator, data) catch {
            return err.OutOfMemory;
        };
    }

    /// 写入单个字节
    fn writeByte(*_, b: u8): ! {
        const one: [1]u8 = [b];
        _.write(one[0..1])?;
    }

    /// 标记已写出/消费 `amount` 字节（超出未读长度时安全截断并自动归零收缩）
    fn wrote(*_, amount: usize) {
        if (amount == 0) {
            return;
        }
        const rem = _.size();
        if (amount >= rem) {
            _.reset();
            return;
        }
        _.cursor += amount;
    }

    /// 将最多 `out.len` 字节拷贝到调用方缓冲并推进游标，返回实际读取字节数
    fn consume(*_, out: []u8): usize {
        const avail = _.slice();
        if (avail.len == 0 || out.len == 0) {
            return 0;
        }
        const n = if (avail.len < out.len) avail.len else out.len;
        let i: usize = 0;
        while (i < n) {
            out[i] = avail[i];
            i += 1;
        }
        _.wrote(n);
        return n;
    }

    /// 若底层容量超过 `shrink_threshold` 且缓冲区为空，则收缩释放多余内存
    fn maybeShrink(*_) {
        if (_.size() == 0 && _.list.capacity > _.shrink_threshold) {
            _.list.shrinkAndFree(_.allocator, _.shrink_threshold);
        }
    }

    /// 清空未消费内容，必要时收缩容量
    fn reset(*_) {
        _.cursor = 0;
        _.list.clearRetainingCapacity();
        _.maybeShrink();
    }
}

/// 线程安全 HTTP 流缓冲区（Mutex 保护 + 排空回调 + 高水位背压检测）
///
/// 典型用法：
/// - 方式 A（细粒度操作）：调用 `write` / `read` / `size` / `reportDrain` 等原子方法；
/// - 方式 B（批量零拷贝操作，与 Bun 原版一致）：调用 `acquire()` 获取 `*StreamBuffer`，
///   操作完成后务必调用 `release()` 释放互斥锁。
pub struct ThreadSafeStreamBuffer {
    buffer: StreamBuffer,
    mutex: std.Thread.Mutex = .{},
    drain_callback: StreamDrainCallback? = null,
    high_water_mark: usize = 0,
    drain_count: u64 = 0,

    /// 创建线程安全流缓冲区（无上限）
    static fn init(allocator: std.mem.Allocator): ThreadSafeStreamBuffer {
        return ThreadSafeStreamBuffer{
            buffer: StreamBuffer.init(allocator),
        };
    }

    /// 创建带容量上限与高水位线背压阈值的线程安全流缓冲区
    static fn initBounded(
        allocator: std.mem.Allocator,
        max_bytes: usize,
        high_water_mark: usize,
    ): ThreadSafeStreamBuffer {
        return ThreadSafeStreamBuffer{
            buffer: StreamBuffer.initBounded(allocator, max_bytes),
            high_water_mark: high_water_mark,
        };
    }

    /// RAII 确定性析构函数
    ~init() {
        _.buffer.deinit();
        _.drain_callback = null;
    }

    /// 显式加锁释放内部缓冲区并清空排空回调
    fn deinit(*_) {
        _.mutex.lock();
        _.buffer.deinit();
        _.drain_callback = null;
        _.mutex.unlock();
    }

    /// 获取互斥锁并返回内部 `*StreamBuffer` 指针（必须与 `release()` 配对使用）
    fn acquire(*_): *StreamBuffer {
        _.mutex.lock();
        return &_.buffer;
    }

    /// 释放由 `acquire()` 获取的互斥锁
    fn release(*_) {
        _.mutex.unlock();
    }

    /// 线程安全写入字节切片
    fn write(*_, data: []const u8): ! {
        _.mutex.lock();
        defer _.mutex.unlock();
        _.buffer.write(data)?;
    }

    /// 线程安全读取并消费最多 `out.len` 字节
    fn read(*_, out: []u8): usize {
        _.mutex.lock();
        defer _.mutex.unlock();
        return _.buffer.consume(out);
    }

    /// 线程安全清空并收缩缓冲区
    fn reset(*_) {
        _.mutex.lock();
        defer _.mutex.unlock();
        _.buffer.reset();
    }

    /// 线程安全查询当前未消费字节数
    fn size(*_): usize {
        _.mutex.lock();
        defer _.mutex.unlock();
        return _.buffer.size();
    }

    /// 线程安全查询缓冲区是否为空
    fn isEmpty(*_): bool {
        _.mutex.lock();
        defer _.mutex.unlock();
        return _.buffer.isEmpty();
    }

    /// 线程安全查询当前内存占用容量
    fn memoryCost(*_): usize {
        _.mutex.lock();
        defer _.mutex.unlock();
        return _.buffer.memoryCost();
    }

    /// 线程安全查询是否处于高水位背压状态
    fn isBackpressured(*_): bool {
        _.mutex.lock();
        defer _.mutex.unlock();
        return _.buffer.isHighWater(_.high_water_mark);
    }

    /// 注册缓冲区排空回调
    fn setDrainCallback(*_, cb: StreamDrainCallback) {
        _.mutex.lock();
        defer _.mutex.unlock();
        _.drain_callback = cb;
    }

    /// 清除缓冲区排空回调
    fn clearDrainCallback(*_) {
        _.mutex.lock();
        defer _.mutex.unlock();
        _.drain_callback = null;
    }

    /// 若缓冲区已排空且注册了回调，则在**解锁后**安全触发回调（防止回调重入死锁），返回是否触发
    fn reportDrain(*_): bool {
        let cb_to_call: StreamDrainCallback? = null;
        _.mutex.lock();
        if (_.buffer.isEmpty()) {
            if (cb := _.drain_callback) {
                cb_to_call = cb;
                _.drain_count += 1;
            }
        }
        _.mutex.unlock();

        if (cb := cb_to_call) {
            cb.call();
            return true;
        }
        return false;
    }

    /// 查询累计触发排空回调的次数
    fn drainCount(*_): u64 {
        _.mutex.lock();
        defer _.mutex.unlock();
        return _.drain_count;
    }
}
