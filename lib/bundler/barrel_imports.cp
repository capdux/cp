// Source: Absorbed from /data/dev/bun/src/bundler/barrel_imports.cp | Synced: 2026-11-21T20:01:31Z
// Pure-CP zero-allocation Barrel File Optimization Engine (inspired by Bun & Rolldown):
// - Detects pure re-export barrel modules (`sideEffects: false` / explicit allowlist)
// - Tracks persistent `RequestedExportsSet` (`partial` vs `all`) with deduplication
// - Defers unused submodule import records while never deferring `export *` records
// - Supports incremental export request wakeups & downstream alias/star propagation

/// 单模块最大追踪导出请求数 / 重导出规则数 / 导入记录位图上限（64 条，零堆分配）
pub const MAX_BARREL_REQUESTED_EXPORTS: usize = 64;
pub const MAX_BARREL_REEXPORTS: usize = 64;
pub const MAX_BARREL_IMPORT_RECORDS: usize = 64;

/// Barrel 优化器错误集
pub err BarrelError {
    RequestedCapacityExceeded,
    ReExportCapacityExceeded,
    ImportRecordOutOfRange,
}

/// 导出请求集合模式（对齐 Bun `RequestedExports`）
pub enum RequestedExportsKind: u8 {
    /// 仅请求了部分具名导出
    partial = 0,
    /// 请求了全量导出（如 `import * as ns` 或副作用加载）
    all = 1,
}

/// 单条具名重导出映射（如 `export { original_alias as exported_alias } from <import_record_index>`）
pub struct BarrelReExport {
    exported_alias: string = "",
    original_alias: string? = null,
    import_record_index: u32 = 0,
    /// 当底层导入为 `import * as ns; export { ns }` 时为 true，传播时需将目标升级为全量请求
    alias_is_star: bool = false,
}

/// 重导出解析结果（对齐 Bun `BarrelExportResolution`）
pub struct BarrelExportResolution {
    import_record_index: u32,
    original_alias: string?,
    alias_is_star: bool,
}

/// 持久化导出请求追踪集（支持跨调用去重与环路检测，零堆分配）
pub struct RequestedExportsSet {
    kind: RequestedExportsKind = .partial,
    aliases: [MAX_BARREL_REQUESTED_EXPORTS]string = [""; MAX_BARREL_REQUESTED_EXPORTS],
    len: usize = 0,

    static fn init(): RequestedExportsSet {
        return RequestedExportsSet{
            kind: .partial,
            aliases: [""; MAX_BARREL_REQUESTED_EXPORTS],
            len: 0,
        };
    }

    fn isAll(): bool {
        return _.kind == .all;
    }

    fn contains(alias: string): bool {
        if (_.kind == .all) {
            return true;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (std.mem.eql(u8, _.aliases[i], alias)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 升级为全量请求；若原本尚未是 `.all` 返回 `true`（表示有新增变化），否则返回 `false`
    fn recordAll(*_): bool {
        if (_.kind == .all) {
            return false;
        }
        _.kind = .all;
        return true;
    }

    /// 记录单个具名导出请求；若为新增请求返回 `true`，已存在或已是 `.all` 返回 `false`（去重/防环）
    fn recordPartial(*_, alias: string): bool! {
        if (_.kind == .all) {
            return false;
        }
        if (_.contains(alias)) {
            return false;
        }
        if (_.len >= MAX_BARREL_REQUESTED_EXPORTS) {
            return err.RequestedCapacityExceeded;
        }
        _.aliases[_.len] = alias;
        _.len += 1;
        return true;
    }
}

/// Barrel 优化资格检查元数据
pub struct BarrelEligibility {
    is_explicit_optimize: bool = false,
    side_effects_false: bool = false,
    has_local_definitions: bool = false,
    is_export_star_target: bool = false,
    import_record_count: u32 = 0,
    reexport_count: u32 = 0,
    export_star_count: u32 = 0,

    /// 严格对齐 Bun `applyBarrelOptimizationImpl` 资格判定：
    /// 1. 显式白名单或 `sideEffects: false`
    /// 2. 至少有 1 条 import record 且至少有 1 条重导出或 `export *`
    /// 3. 纯重导出（无本地定义）
    /// 4. 不是其他 barrel 的 `export *` 目标
    fn isEligible(): bool {
        if (!_.is_explicit_optimize && !_.side_effects_false) {
            return false;
        }
        if (_.import_record_count == 0) {
            return false;
        }
        if (_.reexport_count == 0 && _.export_star_count == 0) {
            return false;
        }
        if (_.has_local_definitions) {
            return false;
        }
        if (_.is_export_star_target) {
            return false;
        }
        return true;
    }
}

/// Barrel 模块优化调度结果（64 位位图精确表示每条 import record 的激活/延迟状态）
pub struct BarrelPlanResult {
    /// 需要立即加载的 import record 位图（bit i == 1 表示 record i 活跃）
    active_mask: u64,
    /// 可安全延迟跳过加载的 import record 位图（bit i == 1 表示 record i 被延迟）
    deferred_mask: u64,
    active_count: u32,
    deferred_count: u32,

    fn isActive(record_idx: u32): bool {
        if (record_idx >= MAX_BARREL_IMPORT_RECORDS) {
            return false;
        }
        const shift: u6 = @intCast(record_idx);
        return ((_.active_mask >> shift) & 1) != 0;
    }

    fn isDeferred(record_idx: u32): bool {
        if (record_idx >= MAX_BARREL_IMPORT_RECORDS) {
            return false;
        }
        const shift: u6 = @intCast(record_idx);
        return ((_.deferred_mask >> shift) & 1) != 0;
    }
}

/// 增量导出请求唤醒结果
pub struct BarrelWakeupResult {
    /// 是否触发了新的模块加载（去重后仅首次请求为 true）
    newly_activated: bool,
    /// 目标 import record 索引（若匹配到具名重导出）
    import_record_index: u32?,
    /// 需向下游传播的原始别名（例如 `export { d as c }` -> `"d"`）
    propagate_alias: string?,
    /// 若底层为 `import * as ns` 或全量请求，下游需按 `.all` 传播
    propagate_all: bool,
}

/// 单个 Barrel 文件的纯重导出映射表与按需延迟引擎
pub struct BarrelModule {
    import_record_count: u32 = 0,
    reexports: [MAX_BARREL_REEXPORTS]BarrelReExport = [BarrelReExport{}; MAX_BARREL_REEXPORTS],
    reexport_len: usize = 0,
    /// `export * from ...` 对应的 import record 位图（永不延迟以防循环竞态）
    export_star_mask: u64 = 0,
    /// 当前已激活加载的 import record 位图
    activated_mask: u64 = 0,
    requested: RequestedExportsSet = RequestedExportsSet.init(),

    static fn init(import_record_count: u32): BarrelModule! {
        if (import_record_count > MAX_BARREL_IMPORT_RECORDS) {
            return err.ImportRecordOutOfRange;
        }
        return BarrelModule{
            import_record_count: import_record_count,
            reexports: [BarrelReExport{}; MAX_BARREL_REEXPORTS],
            reexport_len: 0,
            export_star_mask: 0,
            activated_mask: 0,
            requested: RequestedExportsSet.init(),
        };
    }

    fn addReExport(
        *_,
        exported_alias: string,
        original_alias: string?,
        import_record_index: u32,
        alias_is_star: bool,
    ): ! {
        if (import_record_index >= _.import_record_count) {
            return err.ImportRecordOutOfRange;
        }
        if (_.reexport_len >= MAX_BARREL_REEXPORTS) {
            return err.ReExportCapacityExceeded;
        }
        _.reexports[_.reexport_len] = BarrelReExport{
            exported_alias: exported_alias,
            original_alias: original_alias,
            import_record_index: import_record_index,
            alias_is_star: alias_is_star,
        };
        _.reexport_len += 1;
    }

    fn markExportStar(*_, import_record_index: u32): ! {
        if (import_record_index >= _.import_record_count) {
            return err.ImportRecordOutOfRange;
        }
        const shift: u6 = @intCast(import_record_index);
        _.export_star_mask |= (@as(u64, 1) << shift);
    }

    /// 按导出别名解析对应的 `BarrelExportResolution`（对齐 Bun `resolveBarrelExport`）
    fn resolveExport(alias: string): BarrelExportResolution? {
        let i: usize = 0;
        while (i < _.reexport_len) {
            if (std.mem.eql(u8, _.reexports[i].exported_alias, alias)) {
                return BarrelExportResolution{
                    import_record_index: _.reexports[i].import_record_index,
                    original_alias: _.reexports[i].original_alias,
                    alias_is_star: _.reexports[i].alias_is_star,
                };
            }
            i += 1;
        }
        return null;
    }

    /// 计算当前 Barrel 的激活/延迟加载计划（对齐 Bun `applyBarrelOptimizationImpl`）
    fn computePlan(*_): BarrelPlanResult {
        let all_mask: u64 = 0;
        let idx: u32 = 0;
        while (idx < _.import_record_count) {
            const shift: u6 = @intCast(idx);
            all_mask |= (@as(u64, 1) << shift);
            idx += 1;
        }

        // `export *` records are never deferred
        let active: u64 = _.export_star_mask | _.activated_mask;

        if (_.requested.isAll()) {
            active = all_mask;
        } else {
            let i: usize = 0;
            while (i < _.requested.len) {
                if (res := _.resolveExport(_.requested.aliases[i])) {
                    const shift: u6 = @intCast(res.import_record_index);
                    active |= (@as(u64, 1) << shift);
                }
                i += 1;
            }
        }

        _.activated_mask = active;
        const deferred = all_mask & (~active);

        let active_cnt: u32 = 0;
        let deferred_cnt: u32 = 0;
        idx = 0;
        while (idx < _.import_record_count) {
            const shift: u6 = @intCast(idx);
            if (((active >> shift) & 1) != 0) {
                active_cnt += 1;
            } else {
                deferred_cnt += 1;
            }
            idx += 1;
        }

        return BarrelPlanResult{
            active_mask: active,
            deferred_mask: deferred,
            active_count: active_cnt,
            deferred_count: deferred_cnt,
        };
    }

    /// 增量请求某个具名导出：自动去重，并在目标子模块尚未加载时将其唤醒
    fn requestExport(*_, alias: string): BarrelWakeupResult! {
        const is_new = _.requested.recordPartial(alias)?;
        if (!is_new) {
            return BarrelWakeupResult{
                newly_activated: false,
                import_record_index: null,
                propagate_alias: null,
                propagate_all: false,
            };
        }
        if (res := _.resolveExport(alias)) {
            const shift: u6 = @intCast(res.import_record_index);
            const bit = (@as(u64, 1) << shift);
            const was_active = (_.activated_mask & bit) != 0;
            _.activated_mask |= bit;
            return BarrelWakeupResult{
                newly_activated: !was_active,
                import_record_index: res.import_record_index,
                propagate_alias: res.original_alias,
                propagate_all: res.alias_is_star,
            };
        }
        return BarrelWakeupResult{
            newly_activated: false,
            import_record_index: null,
            propagate_alias: null,
            propagate_all: false,
        };
    }

    /// 将 Barrel 升级为全量请求（例如出现 `import * as lib from 'pkg'`），返回最新全量激活计划
    fn requestAll(*_): BarrelPlanResult {
        * = _.requested.recordAll();
        return _.computePlan();
    }
}
