// Source: Absorbed from /data/dev/bun/src/shell/AllocScope.cp | Synced: 2026-11-14T19:14:45Z
// Pure-CP zero-allocation scoped allocation ownership & leak tracker
// (mirroring Bun shell/AllocScope.cp beginScope/endScope/leakSlice/assertInScope).

/// 单作用域最大并发活跃分配追踪槽位数
pub const ALLOC_SCOPE_MAX_ENTRIES: usize = 64;

/// 作用域内存追踪错误集
pub err AllocScopeError {
    ScopeTableFull,
    InvalidAllocation,
    DuplicateAllocation,
    NotOwnedByScope,
    DoubleFree,
    LeaksDetected,
}

/// 单条活跃分配追踪记录
pub struct AllocTrackEntry {
    alloc_id: u64 = 0,
    size: usize = 0,
    tag: u32 = 0,
    active: bool = false,
}

/// 作用域结束/巡检汇总报告
pub struct AllocScopeReport {
    leaked_count: usize = 0,
    leaked_bytes: usize = 0,
    total_allocs: usize = 0,
    total_frees: usize = 0,
    external_transfers: usize = 0,
    peak_bytes: usize = 0,

    fn isClean(): bool {
        return _.leaked_count == 0 && _.leaked_bytes == 0;
    }
}

/// 零动态堆分配的作用域内存所有权与泄漏检测器（对齐 Bun `shell/AllocScope.cp`）
///
/// - 调试模式（`enabled = true`）：精确追踪每次 `trackAlloc` / `trackFree` / `leakSlice` 与 `assertInScope`，
///   在 `endScope()` 时自动拦截遗留未释放项并返回 `AllocScopeError.LeaksDetected`；
/// - 发布模式（`enabled = false`）：所有追踪操作均降级为零开销 no-op。
pub struct AllocScope {
    entries: [64]AllocTrackEntry = [AllocTrackEntry{}; 64],
    enabled: bool = true,
    live_count: usize = 0,
    live_bytes: usize = 0,
    peak_bytes: usize = 0,
    total_allocs: usize = 0,
    total_frees: usize = 0,
    external_transfers: usize = 0,

    /// 开启一个新的分配追踪作用域（对齐 Bun `AllocScope.beginScope`）
    static fn beginScope(enabled: bool): AllocScope {
        return AllocScope{
            enabled: enabled,
        };
    }

    fn isEnabled(): bool {
        return _.enabled;
    }

    fn liveCount(): usize {
        return _.live_count;
    }

    fn liveBytes(): usize {
        return _.live_bytes;
    }

    fn peakBytes(): usize {
        return _.peak_bytes;
    }

    fn findActiveIndex(alloc_id: u64): usize? {
        if (alloc_id == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < ALLOC_SCOPE_MAX_ENTRIES) {
            if (_.entries[i].active && _.entries[i].alloc_id == alloc_id) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn isOwned(alloc_id: u64): bool {
        if (!_.enabled) {
            return true;
        }
        return _.findActiveIndex(alloc_id) != null;
    }

    /// 登记一笔受当前作用域监管的分配
    fn trackAlloc(*_, alloc_id: u64, size: usize, tag: u32): bool! {
        if (!_.enabled) {
            return false;
        }
        if (alloc_id == 0) {
            return err.InvalidAllocation;
        }
        if (_.findActiveIndex(alloc_id) != null) {
            return err.DuplicateAllocation;
        }
        let i: usize = 0;
        while (i < ALLOC_SCOPE_MAX_ENTRIES) {
            if (!_.entries[i].active) {
                _.entries[i] = AllocTrackEntry{
                    alloc_id: alloc_id,
                    size: size,
                    tag: tag,
                    active: true,
                };
                _.live_count += 1;
                _.live_bytes += size;
                _.total_allocs += 1;
                if (_.live_bytes > _.peak_bytes) {
                    _.peak_bytes = _.live_bytes;
                }
                return true;
            }
            i += 1;
        }
        return err.ScopeTableFull;
    }

    /// 释放已登记的分配并返回其字节数（重复释放或未知 ID 返回 `DoubleFree`）
    fn trackFree(*_, alloc_id: u64): usize! {
        if (!_.enabled) {
            return 0;
        }
        if (alloc_id == 0) {
            return err.InvalidAllocation;
        }
        if (idx := _.findActiveIndex(alloc_id)) {
            const freed = _.entries[idx].size;
            _.entries[idx].active = false;
            _.entries[idx].alloc_id = 0;
            _.entries[idx].size = 0;
            _.entries[idx].tag = 0;
            _.live_count -= 1;
            _.live_bytes -= freed;
            _.total_frees += 1;
            return freed;
        }
        return err.DoubleFree;
    }

    /// 显式移交所有权给外部作用域（对齐 Bun `AllocScope.leakSlice` / `trackExternalFree`）
    fn leakSlice(*_, alloc_id: u64): usize! {
        if (!_.enabled) {
            return 0;
        }
        if (alloc_id == 0) {
            return err.InvalidAllocation;
        }
        if (idx := _.findActiveIndex(alloc_id)) {
            const transferred = _.entries[idx].size;
            _.entries[idx].active = false;
            _.entries[idx].alloc_id = 0;
            _.entries[idx].size = 0;
            _.entries[idx].tag = 0;
            _.live_count -= 1;
            _.live_bytes -= transferred;
            _.external_transfers += 1;
            return transferred;
        }
        return err.NotOwnedByScope;
    }

    /// 断言指定分配当前归属本作用域（对齐 Bun `AllocScope.assertInScope`）
    fn assertInScope(alloc_id: u64): bool! {
        if (!_.enabled) {
            return true;
        }
        if (_.findActiveIndex(alloc_id) == null) {
            return err.NotOwnedByScope;
        }
        return true;
    }

    /// 获取当前作用域的实时统计报告
    fn report(): AllocScopeReport {
        return AllocScopeReport{
            leaked_count: _.live_count,
            leaked_bytes: _.live_bytes,
            total_allocs: _.total_allocs,
            total_frees: _.total_frees,
            external_transfers: _.external_transfers,
            peak_bytes: _.peak_bytes,
        };
    }

    /// 结束当前作用域并检查泄漏（对齐 Bun `AllocScope.endScope`）
    fn endScope(*_): AllocScopeReport! {
        const rep = _.report();
        if (!_.enabled) {
            return rep;
        }
        if (rep.leaked_count > 0) {
            return err.LeaksDetected;
        }
        return rep;
    }
}
