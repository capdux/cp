// Source: Absorbed from /data/dev/bun/src/shell/EnvStr.cp | Synced: 2026-11-22T14:28:00Z
// Pure-CP zero-allocation Shell Environment String (`EnvStr`) & Reference-Counted String Pool.
// Aligned with Bun `src/shell/EnvStr.cp` and `docs/runtime/shell.mdx`:
// - Three-state representation (`Empty`, `Slice`, `RefCounted`):
//   * Zero-length strings collapse to `.Empty` without allocating a pool slot.
//   * Static/AST strings use zero-copy borrowed `.Slice` with zero refcount overhead.
//   * Dynamic environment values use `.RefCounted` pool handles shared across subshells
//     and pipeline stages without deep-copying byte payloads.
// - `memoryCost()` amortizes refcounted byte size by `len / ref_count` (matching Bun's GC
//   memory accounting formula).
// - Protects against double-release (`AlreadyReleased`) and invalid handle access.

/// 引用计数字符串池最大槽位数
pub const ENV_STR_MAX_SLOTS: usize = 64;
/// 引用计数字符串池紧凑字节缓冲区容量
pub const ENV_STR_ARENA_CAP: usize = 4096;
/// 无效池槽位哨兵值
pub const ENV_STR_INVALID_SLOT: u32 = 0xFFFF_FFFF;

/// 环境字符串池错误集
pub err EnvStrError {
    SlotTableFull,
    ArenaFull,
    InvalidSlot,
    AlreadyReleased,
}

/// 环境字符串存储类别（对齐 Bun `EnvStr.Tag`）
pub enum EnvStrTag: u8 {
    Empty = 0,
    Slice = 1,
    RefCounted = 2,
}

/// 零拷贝 / 引用计数混合环境字符串句柄（对齐 Bun `EnvStr`）
pub struct EnvStr {
    tag: EnvStrTag = .Empty,
    borrowed: []const u8 = "",
    slot_id: u32 = ENV_STR_INVALID_SLOT,
    len: usize = 0,

    /// 构造空环境字符串
    static fn empty(): EnvStr {
        return EnvStr{};
    }

    /// 从借用切片构造 `EnvStr`（空串自动折叠为 `.Empty`）
    static fn initSlice(str: []const u8): EnvStr {
        if (str.len == 0) {
            return EnvStr{};
        }
        return EnvStr{
            tag: .Slice,
            borrowed: str,
            slot_id: ENV_STR_INVALID_SLOT,
            len: str.len,
        };
    }

    /// 返回是否为 `.Empty`
    fn isEmpty(): bool {
        return _.tag == .Empty || _.len == 0;
    }

    /// 返回是否为引用计数托管字符串
    fn isRefCounted(): bool {
        return _.tag == .RefCounted;
    }
}

/// 池内单个引用计数字符串槽位元数据
pub struct EnvStrSlot {
    in_use: bool = false,
    released: bool = false,
    ref_count: u32 = 0,
    offset: usize = 0,
    len: usize = 0,
}

/// 零堆分配引用计数环境字符串池
pub struct EnvStrPool {
    slots: [ENV_STR_MAX_SLOTS]EnvStrSlot = [EnvStrSlot{}; ENV_STR_MAX_SLOTS],
    arena: [ENV_STR_ARENA_CAP]u8 = [0; ENV_STR_ARENA_CAP],
    arena_used: usize = 0,
    live_slots: usize = 0,

    static fn init(): EnvStrPool {
        return EnvStrPool{};
    }

    /// 复制字节到池内并创建 `ref_count = 1` 的引用计数 `EnvStr`（空串直接返回 `.Empty`）
    fn dupeRefCounted(*_, raw: []const u8): EnvStr! {
        if (raw.len == 0) {
            return EnvStr.empty();
        }
        if (_.arena_used + raw.len > ENV_STR_ARENA_CAP) {
            return err.ArenaFull;
        }
        let found: usize? = null;
        for (i of 0..ENV_STR_MAX_SLOTS) {
            if (!_.slots[i].in_use) {
                found = i;
                break;
            }
        }
        const slot_idx = found ?? {
            return err.SlotTableFull;
        };

        const off = _.arena_used;
        for (k of 0..raw.len) {
            _.arena[off + k] = raw[k];
        }
        _.arena_used += raw.len;

        _.slots[slot_idx] = EnvStrSlot{
            in_use: true,
            released: false,
            ref_count: 1,
            offset: off,
            len: raw.len,
        };
        _.live_slots += 1;

        return EnvStr{
            tag: .RefCounted,
            borrowed: "",
            slot_id: slot_idx as u32,
            len: raw.len,
        };
    }

    /// 读取任意 `EnvStr` 的只读字节切片（`.Empty` 返回 `""`）
    fn slice(s: EnvStr): ([]const u8)! {
        return switch (s.tag) {
            .Empty => "",
            .Slice => s.borrowed,
            .RefCounted => {
                const idx = s.slot_id as usize;
                if (s.slot_id == ENV_STR_INVALID_SLOT || idx >= ENV_STR_MAX_SLOTS || !_.slots[idx].in_use) {
                    return err.InvalidSlot;
                }
                if (_.slots[idx].released) {
                    return err.AlreadyReleased;
                }
                const sl = _.slots[idx];
                return _.arena[sl.offset .. sl.offset + sl.len];
            },
        };
    }

    /// 递增引用计数（`.Empty` 与 `.Slice` 为零开销 no-op，返回 `0`）
    fn ref(*_, s: EnvStr): u32! {
        if (s.tag != .RefCounted) {
            return 0;
        }
        const idx = s.slot_id as usize;
        if (s.slot_id == ENV_STR_INVALID_SLOT || idx >= ENV_STR_MAX_SLOTS || !_.slots[idx].in_use) {
            return err.InvalidSlot;
        }
        if (_.slots[idx].released) {
            return err.AlreadyReleased;
        }
        _.slots[idx].ref_count += 1;
        return _.slots[idx].ref_count;
    }

    /// 递减引用计数；归零时标记槽位已释放并减少 `live_slots`（`.Empty`/`.Slice` 为 no-op）
    fn deref(*_, s: EnvStr): u32! {
        if (s.tag != .RefCounted) {
            return 0;
        }
        const idx = s.slot_id as usize;
        if (s.slot_id == ENV_STR_INVALID_SLOT || idx >= ENV_STR_MAX_SLOTS || !_.slots[idx].in_use) {
            return err.InvalidSlot;
        }
        if (_.slots[idx].released || _.slots[idx].ref_count == 0) {
            return err.AlreadyReleased;
        }
        _.slots[idx].ref_count -= 1;
        if (_.slots[idx].ref_count == 0) {
            _.slots[idx].released = true;
            if (_.live_slots > 0) {
                _.live_slots -= 1;
            }
        }
        return _.slots[idx].ref_count;
    }

    /// 返回引用计数句柄的当前引用计数（`.Empty`/`.Slice` 返回 `0`）
    fn refCount(s: EnvStr): u32! {
        if (s.tag != .RefCounted) return 0;
        const idx = s.slot_id as usize;
        if (s.slot_id == ENV_STR_INVALID_SLOT || idx >= ENV_STR_MAX_SLOTS || !_.slots[idx].in_use) {
            return err.InvalidSlot;
        }
        return _.slots[idx].ref_count;
    }

    /// 计算分摊后的内存开销（对齐 Bun `EnvStr.memoryCost`：`len / ref_count`）
    fn memoryCost(s: EnvStr): usize {
        return switch (s.tag) {
            .Empty => 0,
            .Slice => s.len,
            .RefCounted => {
                const idx = s.slot_id as usize;
                if (s.slot_id == ENV_STR_INVALID_SLOT || idx >= ENV_STR_MAX_SLOTS || !_.slots[idx].in_use) {
                    return 0;
                }
                const rc = _.slots[idx].ref_count as usize;
                if (rc == 0) return 0;
                return s.len / rc;
            },
        };
    }

    /// 比较两个 `EnvStr` 的字节内容是否完全相等（跨 `Slice` / `RefCounted` / `Empty`）
    fn eql(a: EnvStr, b: EnvStr): bool! {
        if (a.len != b.len) return false;
        const sa = _.slice(a)?;
        const sb = _.slice(b)?;
        for (i of 0..sa.len) {
            if (sa[i] != sb[i]) return false;
        }
        return true;
    }
}
