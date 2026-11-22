// Source: Absorbed from /data/dev/bun/src/shell/RefCountedStr.cp | Synced: 2026-11-22T20:44:16Z
// Pure-CP zero-allocation generation-checked reference-counted string pool
// for sharing Shell environment values across subshells without heap allocation.

/// 字符串池最大并发存活槽位数
pub const REF_STR_MAX_SLOTS: usize = 64;

/// 单个引用计数字符串最大字节长度
pub const REF_STR_MAX_LEN: usize = 256;

/// 引用计数字符串池错误集
pub err RefStrError {
    PoolFull,
    StringTooLong,
    InvalidHandle,
    StaleHandle,
    RefCountOverflow,
}

/// 4 字节紧凑引用计数字符串句柄（含代次号防释放后悬垂访问 / ABA 问题）
pub struct RefStrHandle {
    index: u16 = 0,
    generation: u16 = 0,

    fn eql(other: RefStrHandle): bool {
        return _.index == other.index && _.generation == other.generation;
    }
}

/// 单个固定容量引用计数字符串槽位
pub struct RefStrSlot {
    data: [256]u8 = [0; 256],
    len: u16 = 0,
    refcount: u32 = 0,
    generation: u16 = 1,
    active: bool = false,
}

fn eqSlotBytes(slot_data: []const u8, slot_len: usize, text: []const u8): bool {
    if (slot_len != text.len) {
        return false;
    }
    let i: usize = 0;
    while (i < slot_len) {
        if (slot_data[i] != text[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 零动态堆分配的 Shell 引用计数字符串池（对齐 Bun `shell/RefCountedStr.cp`）
///
/// - `alloc(text)` 分配新槽位（初始 `refcount = 1`）；
/// - `intern(text)` 命中相同存活字符串时直接递增 `refcount` 共享槽位；
/// - `ref(h)` / `deref(h)` 严格执行代次校验，`refcount` 归零时自动回收槽位并递增 `generation` 令旧句柄立即失效。
pub struct RefStrPool {
    slots: [64]RefStrSlot = [RefStrSlot{}; 64],
    active_count: usize = 0,

    static fn init(): RefStrPool {
        return RefStrPool{};
    }

    fn activeCount(): usize {
        return _.active_count;
    }

    fn capacity(): usize {
        return REF_STR_MAX_SLOTS;
    }

    fn validateIndex(h: RefStrHandle): usize! {
        const idx = @as(usize, h.index);
        if (idx >= REF_STR_MAX_SLOTS) {
            return err.InvalidHandle;
        }
        if (!_.slots[idx].active || _.slots[idx].generation != h.generation) {
            return err.StaleHandle;
        }
        return idx;
    }

    fn isAlive(h: RefStrHandle): bool {
        const idx = @as(usize, h.index);
        if (idx >= REF_STR_MAX_SLOTS) {
            return false;
        }
        return _.slots[idx].active && _.slots[idx].generation == h.generation;
    }

    /// 分配一个新槽位保存 `text`（`refcount = 1`）
    fn alloc(*_, text: []const u8): RefStrHandle! {
        if (text.len > REF_STR_MAX_LEN) {
            return err.StringTooLong;
        }
        let i: usize = 0;
        while (i < REF_STR_MAX_SLOTS) {
            if (!_.slots[i].active) {
                let k: usize = 0;
                while (k < text.len) {
                    _.slots[i].data[k] = text[k];
                    k += 1;
                }
                _.slots[i].len = @as(u16, @intCast(text.len));
                _.slots[i].refcount = 1;
                _.slots[i].active = true;
                _.active_count += 1;
                return RefStrHandle{
                    index: @as(u16, @intCast(i)),
                    generation: _.slots[i].generation,
                };
            }
            i += 1;
        }
        return err.PoolFull;
    }

    /// 驻留去重：若池中已有相同存活字符串则直接 `refcount += 1` 复用，否则分配新槽位
    fn intern(*_, text: []const u8): RefStrHandle! {
        if (text.len > REF_STR_MAX_LEN) {
            return err.StringTooLong;
        }
        let i: usize = 0;
        while (i < REF_STR_MAX_SLOTS) {
            if (_.slots[i].active) {
                const slen = @as(usize, _.slots[i].len);
                if (eqSlotBytes(_.slots[i].data[0..], slen, text)) {
                    if (_.slots[i].refcount == 0xFFFF_FFFF) {
                        return err.RefCountOverflow;
                    }
                    _.slots[i].refcount += 1;
                    return RefStrHandle{
                        index: @as(u16, @intCast(i)),
                        generation: _.slots[i].generation,
                    };
                }
            }
            i += 1;
        }
        return _.alloc(text);
    }

    /// 递增句柄引用计数并返回新计数
    fn ref(*_, h: RefStrHandle): u32! {
        const idx = _.validateIndex(h)?;
        if (_.slots[idx].refcount == 0xFFFF_FFFF) {
            return err.RefCountOverflow;
        }
        _.slots[idx].refcount += 1;
        return _.slots[idx].refcount;
    }

    /// 递减句柄引用计数；若归零则自动回收槽位、递增代次并返回 `true`
    fn deref(*_, h: RefStrHandle): bool! {
        const idx = _.validateIndex(h)?;
        _.slots[idx].refcount -= 1;
        if (_.slots[idx].refcount == 0) {
            _.slots[idx].active = false;
            _.slots[idx].len = 0;
            _.slots[idx].generation = _.slots[idx].generation +% 1;
            if (_.slots[idx].generation == 0) {
                _.slots[idx].generation = 1;
            }
            _.active_count -= 1;
            return true;
        }
        return false;
    }

    /// 查询句柄当前引用计数
    fn refCount(h: RefStrHandle): u32! {
        const idx = _.validateIndex(h)?;
        return _.slots[idx].refcount;
    }

    /// 零拷贝返回槽位字节切片（对齐 Bun `RefCountedStr.byteSlice()`）
    fn byteSlice(h: RefStrHandle): ([]const u8)! {
        const idx = _.validateIndex(h)?;
        const slen = @as(usize, _.slots[idx].len);
        if (slen == 0) {
            return "";
        }
        return _.slots[idx].data[0..slen];
    }
}
