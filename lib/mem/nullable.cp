// Source: Absorbed from /data/dev/bun/src/bun_alloc/NullableAllocator.cp | Synced: 2026-11-22T20:40:18Z
// Pure-CP zero-allocation NullableBump allocator handle & OwnedOrBorrowed slice wrapper
// (mirroring Bun bun_alloc/NullableAllocator.cp null-safe get/isNull/free & allocator tag checks).

/// 可空分配器错误集
pub err NullableAllocError {
    NullAllocator,
    OutOfMemory,
    InvalidAlignment,
}

/// 切片内存来源标记（区分借用只读切片与分配器自有切片）
pub enum AllocOrigin: u8 {
    Borrowed = 0,
    BumpOwned = 1,
    CustomTag = 2,
}

fn isPowerOfTwo(v: usize): bool {
    return v != 0 && (v & (v - 1)) == 0;
}

fn alignForward(offset: usize, align_bytes: usize): usize {
    const mask = align_bytes - 1;
    return (offset + mask) & (~mask);
}

/// 零动态堆分配的可空栈缓冲分配器（对齐 Bun `NullableAllocator`）
///
/// - 通过 `NullableBump.empty()` 构造空实例时，`isNull() == true`，所有 `free*` 调用均为安全 no-op，
///   `alloc*` / `dupe` 返回 `NullableAllocError.NullAllocator`；
/// - 通过 `NullableBump.init(buf)` / `initWithTag(buf, tag)` 绑定栈缓冲后，支持对齐分配、字节复制、
///   LIFO 尾部回收与自定义来源标签判别（对应 Bun `isWTFAllocator`）。
pub struct NullableBump {
    buf: ([]u8)? = null,
    pos: usize = 0,
    custom_tag: u16 = 0,
    alloc_count: usize = 0,
    free_count: usize = 0,

    /// 构造空分配器（`isNull() == true`）
    static fn empty(): NullableBump {
        return NullableBump{};
    }

    /// 绑定调用方提供的固定栈缓冲
    static fn init(buf: []u8): NullableBump {
        return NullableBump{
            buf: buf,
            pos: 0,
            custom_tag: 0,
        };
    }

    /// 绑定固定栈缓冲并附加自定义分配器标签（对齐 Bun `isWTFAllocator` 判别模式）
    static fn initWithTag(buf: []u8, tag: u16): NullableBump {
        return NullableBump{
            buf: buf,
            pos: 0,
            custom_tag: tag,
        };
    }

    fn isNull(): bool {
        return _.buf == null;
    }

    fn customTag(): u16 {
        return _.custom_tag;
    }

    fn isCustomTag(tag: u16): bool {
        if (_.buf == null) {
            return false;
        }
        return _.custom_tag == tag && tag != 0;
    }

    fn capacity(): usize {
        if (b := _.buf) {
            return b.len;
        }
        return 0;
    }

    fn usedBytes(): usize {
        return _.pos;
    }

    fn remainingBytes(): usize {
        if (b := _.buf) {
            return b.len - _.pos;
        }
        return 0;
    }

    fn allocCount(): usize {
        return _.alloc_count;
    }

    fn freeCount(): usize {
        return _.free_count;
    }

    /// 按 1 字节对齐分配 `len` 字节切片
    fn alloc(*_, len: usize): ([]u8)! {
        return _.allocAligned(len, 1);
    }

    /// 按指定 2 的幂对齐分配 `len` 字节切片
    fn allocAligned(*_, len: usize, align_bytes: usize): ([]u8)! {
        if (!isPowerOfTwo(align_bytes)) {
            return err.InvalidAlignment;
        }
        if (b := _.buf) {
            const aligned = alignForward(_.pos, align_bytes);
            if (aligned < _.pos || aligned > b.len || b.len - aligned < len) {
                return err.OutOfMemory;
            }
            const end = aligned + len;
            _.pos = end;
            _.alloc_count += 1;
            return b[aligned..end];
        }
        return err.NullAllocator;
    }

    /// 将只读字节切片复制到当前分配器缓冲
    fn dupe(*_, src: []const u8): ([]u8)! {
        let dst = _.alloc(src.len)?;
        let i: usize = 0;
        while (i < src.len) {
            dst[i] = src[i];
            i += 1;
        }
        return dst;
    }

    /// 释放通知（空分配器时安全 no-op；非空时递增释放计数）
    fn freeBytes(*_, len: usize) {
        if (_.buf == null) {
            return;
        }
        * = len;
        _.free_count += 1;
    }

    /// 尝试按 LIFO 顺序回收末尾 `len` 字节；若为空分配器或长度越界则返回 `false`
    fn freeTail(*_, len: usize): bool {
        if (_.buf == null) {
            return false;
        }
        if (len > _.pos) {
            return false;
        }
        _.pos -= len;
        _.free_count += 1;
        return true;
    }

    /// 重置分配偏移（保留底层缓冲与标签）
    fn reset(*_) {
        _.pos = 0;
    }
}

/// 统一承载借用只读切片或 `NullableBump` 自有切片的零拷贝视图
pub struct OwnedOrBorrowed {
    data: []const u8 = "",
    origin_kind: AllocOrigin = .Borrowed,

    /// 零拷贝包装外部借用切片（释放时 no-op）
    static fn borrowed(data: []const u8): OwnedOrBorrowed {
        return OwnedOrBorrowed{
            data: data,
            origin_kind: .Borrowed,
        };
    }

    /// 通过 `NullableBump` 复制并持有自有副本
    static fn fromAlloc(alloc: *NullableBump, src: []const u8): OwnedOrBorrowed! {
        const copied = alloc.dupe(src)?;
        const kind = if (alloc.customTag() != 0) AllocOrigin.CustomTag else AllocOrigin.BumpOwned;
        return OwnedOrBorrowed{
            data: copied,
            origin_kind: kind,
        };
    }

    fn slice(): []const u8 {
        return _.data;
    }

    fn len(): usize {
        return _.data.len;
    }

    fn isEmpty(): bool {
        return _.data.len == 0;
    }

    fn isOwned(): bool {
        return _.origin_kind != .Borrowed;
    }

    fn origin(): AllocOrigin {
        return _.origin_kind;
    }

    /// 释放视图：仅自有切片会通知 `alloc.freeBytes`，随后安全复位为借用空串
    fn release(*_, alloc: *NullableBump) {
        if (_.origin_kind != .Borrowed) {
            alloc.freeBytes(_.data.len);
        }
        _.data = "";
        _.origin_kind = .Borrowed;
    }
}
