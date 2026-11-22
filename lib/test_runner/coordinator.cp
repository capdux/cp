// Source: Absorbed from /data/dev/bun/src/cli/test/parallel/Coordinator.cp | Synced: 2026-11-22T06:28:27Z
// Pure-CP zero-allocation parallel test runner coordinator state machine:
// lazy worker scale-up, work-stealing dispatch, `--bail[=N]` early abort,
// worker crash accounting, and interleaved per-file header deduplication.

import { WORK_STEAL_MAX_WORKERS, WorkStealPool } from "./work_stealing";

/// 协调器支持的最大并发 Worker 槽位数
pub const COORDINATOR_MAX_WORKERS: usize = WORK_STEAL_MAX_WORKERS;

/// 协调器记录异常退出（未完成当前测试文件）的最大文件条目数
pub const COORDINATOR_MAX_CRASHED_FILES: usize = 64;

/// 懒扩容默认延迟阈值（毫秒）：所有活跃 Worker 均持续忙碌超过该时长后才扩容剩余槽位
pub const COORDINATOR_DEFAULT_SCALE_UP_MS: i64 = 150;

/// 并行测试协调器错误集
pub err CoordinatorError {
    InvalidWorkerSlot,
    WorkerNotAlive,
    NoFileInflight,
    FileIndexMismatch,
    CrashListFull,
}

/// 单个 Worker 槽位的生命周期状态
pub enum WorkerSlotState: u8 {
    Unspawned = 0,
    Idle = 1,
    Busy = 2,
    ShuttingDown = 3,
    Exited = 4,
}

/// 单个 Worker 槽位的运行态统计与在途文件追踪
pub struct WorkerSlot {
    state: WorkerSlotState = .Unspawned,
    inflight_file: u32? = null,
    dispatched_at_ms: i64 = 0,
    files_completed: u32 = 0,
    failures_reported: u32 = 0,

    fn isAlive(): bool {
        return _.state == .Idle || _.state == .Busy || _.state == .ShuttingDown;
    }

    fn isBusy(): bool {
        return _.state == .Busy && _.inflight_file != null;
    }
}

/// 并行测试协调器配置（对齐 `bun test --parallel --bail --dots`）
pub struct CoordinatorConfig {
    file_count: u32 = 0,
    parallel_limit: u32 = 1,
    scale_up_after_ms: i64 = COORDINATOR_DEFAULT_SCALE_UP_MS,
    bail_threshold: u32 = 0,
    dots_mode: bool = false,
}

/// 调度决策类型
pub enum DispatchDecisionKind: u8 {
    DispatchFile = 0,
    ShutdownWorker = 1,
}

/// 协调器针对空闲 Worker 的单次派发决策
pub struct DispatchDecision {
    kind: DispatchDecisionKind = .ShutdownWorker,
    file_idx: u32 = 0,
    stolen: bool = false,
}

/// 零堆分配并行测试协调器状态机（对齐 Bun `cli/test/parallel/Coordinator.cp`）
///
/// 核心能力：
/// 1. 懒扩容（`shouldScaleUp` / `applyScaleUp`）：极速套件仅用 1 个 Worker 完成，零多余进程启动开销；
///    当所有已启动的活跃 Worker 均持续忙碌超过 `scale_up_after_ms` 且仍有未派发文件时，一次性拉起剩余 Worker；
/// 2. 工作窃取调度（`assignWork`）：复用 `WorkStealPool`，优先本地顺序消费保持目录局部性，
///    空闲后窃取剩余量最大槽位（含未启动预留槽位）的后半段；
/// 3. `--bail` / `--bail=N` 熔断（`onTestResult` / `bailOut`）：失败数达到阈值即标记 `bailed`，
///    立即令空闲 Worker 进入关闭态并停止派发新文件；
/// 4. Worker 异常退出兜底（`onWorkerExited`）：回收在途文件索引计入 `crashed_files` 与失败统计；
/// 5. 并发输出交错去重（`shouldEmitHeader`）：多 Worker 交错回传结果时自动判定是否需要重印文件路径头。
pub struct ParallelCoordinator {
    pool: WorkStealPool = WorkStealPool{},
    workers: [COORDINATOR_MAX_WORKERS]WorkerSlot = [WorkerSlot{}; COORDINATOR_MAX_WORKERS],
    config: CoordinatorConfig = CoordinatorConfig{},
    effective_limit: u32 = 0,
    spawned_count: u32 = 0,
    live_workers: u32 = 0,
    files_done: u32 = 0,
    total_passed: u32 = 0,
    total_failed: u32 = 0,
    total_skipped: u32 = 0,
    bailed: bool = false,
    last_header_idx: u32? = null,
    crashed_files: [COORDINATOR_MAX_CRASHED_FILES]u32 = [0; COORDINATOR_MAX_CRASHED_FILES],
    crashed_count: usize = 0,

    static fn init(cfg: CoordinatorConfig): ParallelCoordinator {
        let limit = cfg.parallel_limit;
        if (limit == 0) {
            limit = 1;
        }
        if (cfg.file_count < limit) {
            limit = cfg.file_count;
        }
        if ((limit as usize) > COORDINATOR_MAX_WORKERS) {
            limit = COORDINATOR_MAX_WORKERS as u32;
        }
        const pool = if (limit > 0) WorkStealPool.partition(cfg.file_count, limit as usize) else WorkStealPool{};
        return ParallelCoordinator{
            pool: pool,
            config: cfg,
            effective_limit: limit,
        };
    }

    /// 所有文件完成（或触发 bail 熔断）且无存活 Worker 时返回 `true`
    fn isDone(): bool {
        return (_.files_done >= _.config.file_count || _.bailed) && _.live_workers == 0;
    }

    fn hasUndispatchedFiles(): bool {
        return _.pool.remainingTotal() > 0;
    }

    fn remainingFiles(): u32 {
        return _.pool.remainingTotal();
    }

    /// 懒启动下一个未启动的 Worker 槽位；若已达并发上限则返回 `null`
    fn spawnNextWorker(*_): usize? {
        if (_.spawned_count >= _.effective_limit) {
            return null;
        }
        const slot_idx = _.spawned_count as usize;
        _.workers[slot_idx] = WorkerSlot{
            state: .Idle,
            inflight_file: null,
            dispatched_at_ms: 0,
            files_completed: 0,
            failures_reported: 0,
        };
        _.spawned_count += 1;
        _.live_workers += 1;
        return slot_idx;
    }

    /// 计算当前时刻应触发懒扩容的新增 Worker 数量（严格对齐 Bun `Coordinator.maybeScaleUp`）
    fn shouldScaleUp(now_ms: i64): u32 {
        if (_.spawned_count >= _.effective_limit) {
            return 0;
        }
        if (_.bailed || !_.hasUndispatchedFiles()) {
            return 0;
        }
        let saw_alive = false;
        let i: usize = 0;
        while (i < (_.spawned_count as usize)) {
            const w = _.workers[i];
            if (w.isAlive()) {
                saw_alive = true;
                if (!w.isBusy()) {
                    return 0;
                }
                if (now_ms - w.dispatched_at_ms < _.config.scale_up_after_ms) {
                    return 0;
                }
            }
            i += 1;
        }
        if (!saw_alive) {
            return 0;
        }
        const undone = if (_.config.file_count > _.files_done) _.config.file_count - _.files_done else 0;
        const want = if (_.effective_limit < undone) _.effective_limit else undone;
        if (want <= _.spawned_count) {
            return 0;
        }
        return want - _.spawned_count;
    }

    /// 执行懒扩容并返回实际新启动的 Worker 数量
    fn applyScaleUp(*_, now_ms: i64): u32 {
        const needed = _.shouldScaleUp(now_ms);
        let spawned: u32 = 0;
        while (spawned < needed) {
            if (_ := _.spawnNextWorker()) {
                spawned += 1;
            } else {
                break;
            }
        }
        return spawned;
    }

    /// 为指定空闲 Worker 派发下一个文件（本地优先 -> 工作窃取 -> 关闭）
    fn assignWork(*_, worker_id: usize, now_ms: i64): DispatchDecision! {
        if (worker_id >= (_.spawned_count as usize)) {
            return err.InvalidWorkerSlot;
        }
        if (!_.workers[worker_id].isAlive()) {
            return err.WorkerNotAlive;
        }
        if (_.bailed) {
            _.workers[worker_id].state = .ShuttingDown;
            return DispatchDecision{ kind: .ShutdownWorker };
        }

        const had_local = !_.pool.ranges[worker_id].isEmpty();
        if (file_idx := _.pool.nextFor(worker_id)) {
            _.workers[worker_id].state = .Busy;
            _.workers[worker_id].inflight_file = file_idx;
            _.workers[worker_id].dispatched_at_ms = now_ms;
            return DispatchDecision{
                kind: .DispatchFile,
                file_idx: file_idx,
                stolen: !had_local,
            };
        }

        _.workers[worker_id].state = .ShuttingDown;
        return DispatchDecision{ kind: .ShutdownWorker };
    }

    /// 触发 bail 熔断，令所有当前空闲的存活 Worker 立即转入 `ShuttingDown`
    fn bailOut(*_) {
        if (_.bailed) {
            return;
        }
        _.bailed = true;
        let i: usize = 0;
        while (i < (_.spawned_count as usize)) {
            if (_.workers[i].state == .Idle) {
                _.workers[i].state = .ShuttingDown;
            }
            i += 1;
        }
    }

    /// 上报单条测试用例结果；若本次失败首次触发 `--bail` 熔断则返回 `true`
    fn onTestResult(*_, worker_id: usize, file_idx: u32, passed: bool, skipped: bool): bool! {
        if (worker_id >= (_.spawned_count as usize)) {
            return err.InvalidWorkerSlot;
        }
        const cur = _.workers[worker_id].inflight_file ?? return err.NoFileInflight;
        if (cur != file_idx) {
            return err.FileIndexMismatch;
        }

        if (skipped) {
            _.total_skipped += 1;
            return false;
        }
        if (passed) {
            _.total_passed += 1;
            return false;
        }

        _.total_failed += 1;
        _.workers[worker_id].failures_reported += 1;
        if (_.config.bail_threshold > 0 && !_.bailed && _.total_failed >= _.config.bail_threshold) {
            _.bailOut();
            return true;
        }
        return false;
    }

    /// 上报 Worker 完成当前在途文件
    fn onFileDone(*_, worker_id: usize, file_idx: u32)! {
        if (worker_id >= (_.spawned_count as usize)) {
            return err.InvalidWorkerSlot;
        }
        const cur = _.workers[worker_id].inflight_file ?? return err.NoFileInflight;
        if (cur != file_idx) {
            return err.FileIndexMismatch;
        }
        _.workers[worker_id].inflight_file = null;
        _.workers[worker_id].files_completed += 1;
        _.workers[worker_id].state = if (_.bailed) .ShuttingDown else .Idle;
        _.files_done += 1;
    }

    /// 上报 Worker 退出；若退出时仍有在途文件（异常终止），记入 `crashed_files` 并返回该文件索引
    fn onWorkerExited(*_, worker_id: usize, abnormal: bool): u32?! {
        if (worker_id >= (_.spawned_count as usize)) {
            return err.InvalidWorkerSlot;
        }
        if (!_.workers[worker_id].isAlive()) {
            return err.WorkerNotAlive;
        }

        let crashed_idx: u32? = null;
        if (inflight := _.workers[worker_id].inflight_file) {
            _.workers[worker_id].inflight_file = null;
            _.files_done += 1;
            if (abnormal) {
                if (_.crashed_count >= COORDINATOR_MAX_CRASHED_FILES) {
                    return err.CrashListFull;
                }
                _.crashed_files[_.crashed_count] = inflight;
                _.crashed_count += 1;
                _.total_failed += 1;
                crashed_idx = inflight;
                if (_.config.bail_threshold > 0 && !_.bailed && _.total_failed >= _.config.bail_threshold) {
                    _.bailOut();
                }
            }
        }

        _.workers[worker_id].state = .Exited;
        if (_.live_workers > 0) {
            _.live_workers -= 1;
        }
        return crashed_idx;
    }

    /// 判定在当前输出模式下是否需要重新打印 `file_idx` 对应的文件路径头（对齐 Bun `ensureHeader`）
    fn shouldEmitHeader(*_, file_idx: u32): bool {
        if (_.config.dots_mode) {
            return false;
        }
        if (last := _.last_header_idx) {
            if (last == file_idx) {
                return false;
            }
        }
        _.last_header_idx = file_idx;
        return true;
    }

    fn crashedFileAt(idx: usize): u32? {
        if (idx >= _.crashed_count) {
            return null;
        }
        return _.crashed_files[idx];
    }
}
