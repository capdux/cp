// Source: Absorbed from /data/dev/bun/src/cli/test/parallel/FileRange.cp | Synced: 2026-11-22T03:55:40Z
// Pure-CP zero-allocation work-stealing range scheduler for parallel test execution:
// - `FileRange`: contiguous `[lo, hi)` file index slice owned by a worker;
//   local dispatch pops from front (cache-hot), stealing takes the back half
//   (`stealBackHalf`) so both owner and thief preserve directory locality.
// - `WorkStealPool`: fixed-capacity zero-heap multi-worker partition & busiest-victim
//   work-stealing dispatcher supporting up to `WORK_STEAL_MAX_WORKERS` (64) workers.

/// 工作窃取调度池支持的最大并发 Worker 数
pub const WORK_STEAL_MAX_WORKERS: usize = 64;

/// 连续半开测试文件索引区间 `[lo, hi)`（对齐 Bun `FileRange`）
pub struct FileRange {
    lo: u32 = 0,
    hi: u32 = 0,

    /// 构造 `[lo, hi)` 区间；若 `lo >= hi` 规范化为空区间 `[lo, lo)`
    static fn init(lo: u32, hi: u32): FileRange {
        if (lo >= hi) {
            return FileRange{ lo: lo, hi: lo };
        }
        return FileRange{ lo: lo, hi: hi };
    }

    /// 返回区间剩余文件数（空区间安全返回 0）
    fn len(): u32 {
        if (_.lo >= _.hi) {
            return 0;
        }
        return _.hi - _.lo;
    }

    /// 判断区间是否为空
    fn isEmpty(): bool {
        return _.lo >= _.hi;
    }

    /// 判断索引 `index` 是否落在当前区间 `[lo, hi)` 内
    fn contains(index: u32): bool {
        return index >= _.lo && index < _.hi;
    }

    /// 从头部（cache-hot 区域）弹出下一个文件索引（对齐 Bun `popFront()`）
    fn popFront(*_): u32? {
        if (_.isEmpty()) {
            return null;
        }
        const cur = _.lo;
        _.lo += 1;
        return cur;
    }

    /// 从尾部弹出单个文件索引
    fn popBack(*_): u32? {
        if (_.isEmpty()) {
            return null;
        }
        _.hi -= 1;
        return _.hi;
    }

    /// 窃取后半段连续区间给窃取者，保留前半段给原持有者（对齐 Bun `stealBackHalf()`）：
    /// - 双方保持目录局部性；
    /// - 当 `len() == 1` 时，将唯一元素整段交给窃取者，原持有者变空。
    fn stealBackHalf(*_): FileRange? {
        if (_.isEmpty()) {
            return null;
        }
        const mid = _.lo + (_.len() / 2);
        const stolen = FileRange{ lo: mid, hi: _.hi };
        _.hi = mid;
        return stolen;
    }
}

/// 零堆分配多 Worker 工作窃取调度池
pub struct WorkStealPool {
    ranges: [WORK_STEAL_MAX_WORKERS]FileRange = [FileRange{}; WORK_STEAL_MAX_WORKERS],
    worker_count: usize = 0,
    total_files: u32 = 0,

    /// 将 `[0, total_files)` 均匀划分给 `worker_count` 个 Worker（自动钳位至 `[1, WORK_STEAL_MAX_WORKERS]`，0 文件时全部为空区间）
    static fn partition(total_files: u32, worker_count: usize): WorkStealPool {
        let pool = WorkStealPool{
            ranges: [FileRange{}; WORK_STEAL_MAX_WORKERS],
            worker_count: 0,
            total_files: total_files,
        };
        if (worker_count == 0) {
            return pool;
        }
        const wc: usize = if (worker_count > WORK_STEAL_MAX_WORKERS) WORK_STEAL_MAX_WORKERS else worker_count;
        pool.worker_count = wc;

        const wc_u32: u32 = @intCast(wc);
        const base: u32 = total_files / wc_u32;
        const rem: u32 = total_files % wc_u32;

        let cursor: u32 = 0;
        let i: usize = 0;
        while (i < wc) {
            const extra: u32 = if (@as(u32, @intCast(i)) < rem) 1 else 0;
            const span = base + extra;
            pool.ranges[i] = FileRange.init(cursor, cursor + span);
            cursor += span;
            i += 1;
        }
        return pool;
    }

    /// 返回指定 Worker 当前持有的区间（越界返回空区间）
    fn rangeOf(worker_id: usize): FileRange {
        if (worker_id >= _.worker_count) {
            return FileRange{};
        }
        return _.ranges[worker_id];
    }

    /// 返回所有 Worker 尚未派发的剩余任务总数
    fn totalRemaining(): u32 {
        let sum: u32 = 0;
        let i: usize = 0;
        while (i < _.worker_count) {
            sum += _.ranges[i].len();
            i += 1;
        }
        return sum;
    }

    /// 显式为 `thief_id` 从当前剩余任务最多的同伴窃取后半段连续区间
    fn stealFromBusiest(*_, thief_id: usize): FileRange? {
        if (thief_id >= _.worker_count) {
            return null;
        }
        let best_idx: usize = 0;
        let best_len: u32 = 0;
        let i: usize = 0;
        while (i < _.worker_count) {
            if (i != thief_id) {
                const l = _.ranges[i].len();
                if (l > best_len) {
                    best_len = l;
                    best_idx = i;
                }
            }
            i += 1;
        }
        if (best_len == 0) {
            return null;
        }
        if (stolen := _.ranges[best_idx].stealBackHalf()) {
            _.ranges[thief_id] = stolen;
            return stolen;
        }
        return null;
    }

    /// 为 `worker_id` 派发下一个文件索引：
    /// 优先从自身区间头部弹出；自身为空时自动向最忙同伴窃取后半段再从头部弹出。
    fn nextFor(*_, worker_id: usize): u32? {
        if (worker_id >= _.worker_count) {
            return null;
        }
        if (idx := _.ranges[worker_id].popFront()) {
            return idx;
        }
        if (_stolen := _.stealFromBusiest(worker_id)) {
            return _.ranges[worker_id].popFront();
        }
        return null;
    }
}
