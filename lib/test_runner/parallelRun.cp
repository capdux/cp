// Source: Absorbed from /data/dev/bun/src/cli/test/parallel/runner.cp | Synced: 2026-11-23T21:09:52Z
// Pure-CP zero-allocation Parallel Test Runner Coordinator/Worker Bootstrap Planner:
// - Computes effective worker count `K = min(parallel, N)` and serial fallback (`K <= 1`)
// - Resolves `scale_up_after_ms` from CLI override or `BUN_TEST_PARALLEL_SCALE_MS` (default 5ms)
// - Lexicographically sorts test files (unless `--randomize`) and partitions into K contiguous ranges
//   `[idx * N / K, (idx + 1) * N / K)` for directory/import cache locality
// - Encodes/decodes the 9-field `file_done` delta counters between worker and coordinator
// - Builds zero-allocation isolated `--test-worker --isolate` argv lists and 1-indexed worker env plans.

/// 生产默认懒扩容阈值（毫秒，对齐 Bun `default_scale_up_after_ms = 5`）
pub const DEFAULT_PARALLEL_SCALE_UP_MS: i64 = 5;
/// `file_done` 帧 9 个 u32 增量计数字段总字节数
pub const WORKER_FILE_DELTA_BYTES: usize = 36;
/// 单个并行规划器支持的最大文件数
pub const PARALLEL_RUN_MAX_FILES: usize = 64;
/// 单个并行规划器支持的最大 Worker 分区数
pub const PARALLEL_RUN_MAX_WORKERS: usize = 32;
/// Worker argv 最大参数个数
pub const WORKER_ARGV_MAX_ARGS: usize = 32;
/// Worker argv 字符内联缓冲容量
pub const WORKER_ARGV_ARENA_CAP: usize = 1024;

/// 并行测试启动规划器错误集
pub err ParallelRunError {
    TooManyFiles,
    TooManyWorkers,
    TooManyArgs,
    TruncatedPayload,
    BufferTooSmall,
    InvalidWorkerId,
}

/// 计算实际有效 Worker 数：`K = min(requested_parallel, file_count)`
pub fn effectiveWorkerCount(requested_parallel: u32, file_count: u32): u32 {
    if (requested_parallel <= file_count) return requested_parallel;
    return file_count;
}

/// 判定是否启用多 Worker 并行池（`K <= 1` 时回退串行路径，对齐 `runAsCoordinator`）
pub fn shouldUseParallelPool(requested_parallel: u32, file_count: u32): bool {
    return effectiveWorkerCount(requested_parallel, file_count) > 1;
}

/// 解析懒扩容等待时间（CLI 优先，其次 `BUN_TEST_PARALLEL_SCALE_MS`，负值钳位至 0）
pub fn resolveScaleUpDelayMs(cli_delay_ms: u32?, env_val: ([]const u8)?): i64 {
    if (d := cli_delay_ms) {
        return @as(i64, d);
    }
    if (s := env_val) {
        if (parsed := parseSignedI64(s)) {
            return if (parsed < 0) 0 else parsed;
        }
    }
    return DEFAULT_PARALLEL_SCALE_UP_MS;
}

/// 单个 Worker 分区范围 `[lo, hi)`（对齐 `FileRange{ lo: idx * N / K, hi: (idx + 1) * N / K }`）
pub struct ParallelSliceRange {
    lo: u32 = 0,
    hi: u32 = 0,

    fn len(): u32 {
        if (_.hi <= _.lo) return 0;
        return _.hi - _.lo;
    }
}

/// 零堆分配文件排序与 K 路连续区间分区器
pub struct ParallelFilePartition {
    files: [PARALLEL_RUN_MAX_FILES]([]const u8) = [""; PARALLEL_RUN_MAX_FILES],
    file_count: usize = 0,
    ranges: [PARALLEL_RUN_MAX_WORKERS]ParallelSliceRange = [ParallelSliceRange{}; PARALLEL_RUN_MAX_WORKERS],
    worker_count: usize = 0,

    /// 按需（`!randomize`）执行 ASCII 字典序稳定排序，并将文件均匀划分为 `K` 个连续区间
    static fn build(input_files: []const ([]const u8), requested_parallel: u32, randomize: bool): ParallelFilePartition! {
        if (input_files.len > PARALLEL_RUN_MAX_FILES) {
            return err.TooManyFiles;
        }
        let part = ParallelFilePartition{ file_count: input_files.len };
        let i: usize = 0;
        while (i < input_files.len) {
            part.files[i] = input_files[i];
            i += 1;
        }

        // 非 randomize 模式按字典序排序以保持同目录导入缓存局部性
        if (!randomize && part.file_count > 1) {
            let a: usize = 1;
            while (a < part.file_count) {
                const key = part.files[a];
                let b: usize = a;
                while (b > 0 && asciiOrderLess(key, part.files[b - 1])) {
                    part.files[b] = part.files[b - 1];
                    b -= 1;
                }
                part.files[b] = key;
                a += 1;
            }
        }

        const n_u32 = @as(u32, @intCast(part.file_count));
        const k_u32 = effectiveWorkerCount(requested_parallel, n_u32);
        if (@as(usize, k_u32) > PARALLEL_RUN_MAX_WORKERS) {
            return err.TooManyWorkers;
        }
        part.worker_count = @as(usize, k_u32);

        if (k_u32 > 0) {
            let w: u32 = 0;
            while (w < k_u32) {
                part.ranges[@as(usize, w)] = ParallelSliceRange{
                    lo: (w * n_u32) / k_u32,
                    hi: ((w + 1) * n_u32) / k_u32,
                };
                w += 1;
            }
        }
        return part;
    }

    fn fileSlice(): []const ([]const u8) {
        return _.files[0.._.file_count];
    }

    fn rangeForWorker(worker_idx: usize): ParallelSliceRange {
        if (worker_idx >= _.worker_count) return ParallelSliceRange{};
        return _.ranges[worker_idx];
    }
}

/// Worker 单文件执行前后的累计摘要快照
pub struct WorkerSummarySnapshot {
    pass: u32 = 0,
    fail: u32 = 0,
    skip: u32 = 0,
    todo: u32 = 0,
    expectations: u32 = 0,
    skipped_because_label: u32 = 0,
    files: u32 = 0,
    unhandled_errors: u32 = 0,
}

/// `file_done` IPC 帧 9 元组增量计数器（对齐 Bun `runner.cp` L147-160）
pub struct WorkerFileDelta {
    file_idx: u32 = 0,
    pass: u32 = 0,
    fail: u32 = 0,
    skip: u32 = 0,
    todo: u32 = 0,
    expectations: u32 = 0,
    skipped_because_label: u32 = 0,
    files: u32 = 0,
    unhandled_errors: u32 = 0,

    static fn diff(file_idx: u32, before: WorkerSummarySnapshot, after: WorkerSummarySnapshot): WorkerFileDelta {
        return WorkerFileDelta{
            file_idx: file_idx,
            pass: satSubU32(after.pass, before.pass),
            fail: satSubU32(after.fail, before.fail),
            skip: satSubU32(after.skip, before.skip),
            todo: satSubU32(after.todo, before.todo),
            expectations: satSubU32(after.expectations, before.expectations),
            skipped_because_label: satSubU32(after.skipped_because_label, before.skipped_because_label),
            files: satSubU32(after.files, before.files),
            unhandled_errors: satSubU32(after.unhandled_errors, before.unhandled_errors),
        };
    }

    fn hasFailures(): bool {
        return _.fail > 0 || _.unhandled_errors > 0;
    }

    fn encodePayload(out: []u8): usize! {
        if (out.len < WORKER_FILE_DELTA_BYTES) {
            return err.BufferTooSmall;
        }
        writeU32Le(out, 0, _.file_idx);
        writeU32Le(out, 4, _.pass);
        writeU32Le(out, 8, _.fail);
        writeU32Le(out, 12, _.skip);
        writeU32Le(out, 16, _.todo);
        writeU32Le(out, 20, _.expectations);
        writeU32Le(out, 24, _.skipped_because_label);
        writeU32Le(out, 28, _.files);
        writeU32Le(out, 32, _.unhandled_errors);
        return WORKER_FILE_DELTA_BYTES;
    }

    static fn decodePayload(buf: []const u8): WorkerFileDelta! {
        if (buf.len < WORKER_FILE_DELTA_BYTES) {
            return err.TruncatedPayload;
        }
        return WorkerFileDelta{
            file_idx: readU32Le(buf, 0),
            pass: readU32Le(buf, 4),
            fail: readU32Le(buf, 8),
            skip: readU32Le(buf, 12),
            todo: readU32Le(buf, 16),
            expectations: readU32Le(buf, 20),
            skipped_because_label: readU32Le(buf, 24),
            files: readU32Le(buf, 28),
            unhandled_errors: readU32Le(buf, 32),
        };
    }
}

/// Worker 启动 CLI 选项（对齐 `buildWorkerArgv`）
pub struct WorkerCliOptions {
    exe_path: []const u8 = "cap",
    default_timeout_ms: u32 = 5000,
    max_concurrency: u32 = 20,
    run_todo: bool = false,
    only: bool = false,
    dots: bool = false,
    only_failures: bool = false,
    update_snapshots: bool = false,
    concurrent: bool = false,
    randomize: bool = false,
    seed: u32? = null,
    repeat_count: u32 = 0,
    retry: u32 = 0,
    filter_pattern: ([]const u8)? = null,
}

/// 单个参数切片位置
pub struct WorkerArgSpan {
    offset: usize = 0,
    len: usize = 0,
}

/// 零堆分配 `--test-worker --isolate` 启动参数列表构造器
pub struct WorkerArgvBuilder {
    arena: [WORKER_ARGV_ARENA_CAP]u8 = [0; WORKER_ARGV_ARENA_CAP],
    arena_len: usize = 0,
    args: [WORKER_ARGV_MAX_ARGS]WorkerArgSpan = [WorkerArgSpan{}; WORKER_ARGV_MAX_ARGS],
    arg_count: usize = 0,

    static fn build(opts: WorkerCliOptions): WorkerArgvBuilder! {
        let b = WorkerArgvBuilder{};
        * = b.pushArg(opts.exe_path)?;
        * = b.pushArg("test")?;
        * = b.pushArg("--test-worker")?;
        * = b.pushArg("--isolate")?;
        * = b.pushKeyU32("--timeout=", opts.default_timeout_ms)?;

        if (opts.run_todo) * = b.pushArg("--todo")?;
        if (opts.only) * = b.pushArg("--only")?;
        if (opts.dots) * = b.pushArg("--dots")?;
        if (opts.only_failures) * = b.pushArg("--only-failures")?;
        if (opts.update_snapshots) * = b.pushArg("--update-snapshots")?;
        if (opts.concurrent) * = b.pushArg("--concurrent")?;
        if (opts.randomize) * = b.pushArg("--randomize")?;
        if (sd := opts.seed) {
            * = b.pushKeyU32("--seed=", sd)?;
        }
        if (opts.repeat_count > 0) {
            * = b.pushKeyU32("--rerun-each=", opts.repeat_count)?;
        }
        if (opts.retry > 0) {
            * = b.pushKeyU32("--retry=", opts.retry)?;
        }
        * = b.pushKeyU32("--max-concurrency=", opts.max_concurrency)?;
        if (pat := opts.filter_pattern) {
            * = b.pushArg("-t")?;
            * = b.pushArg(pat)?;
        }
        return b;
    }

    fn count(): usize {
        return _.arg_count;
    }

    fn argAt(idx: usize): []const u8 {
        if (idx >= _.arg_count) return "";
        const sp = _.args[idx];
        return _.arena[sp.offset .. sp.offset + sp.len];
    }

    fn pushArg(*_, s: []const u8): usize! {
        if (_.arg_count >= WORKER_ARGV_MAX_ARGS) return err.TooManyArgs;
        if (s.len > WORKER_ARGV_ARENA_CAP - _.arena_len) return err.BufferTooSmall;
        const off = _.arena_len;
        copyBytes(_.arena[off .. off + s.len], s);
        _.arena_len += s.len;
        _.args[_.arg_count] = WorkerArgSpan{ offset: off, len: s.len };
        _.arg_count += 1;
        return _.arg_count;
    }

    fn pushKeyU32(*_, prefix: []const u8, val: u32): usize! {
        let num_buf: [16]u8 = [0; 16];
        const nlen = formatU32Dec(val, num_buf[0..]);
        if (_.arg_count >= WORKER_ARGV_MAX_ARGS) return err.TooManyArgs;
        const total = prefix.len + nlen;
        if (total > WORKER_ARGV_ARENA_CAP - _.arena_len) return err.BufferTooSmall;
        const off = _.arena_len;
        copyBytes(_.arena[off .. off + prefix.len], prefix);
        copyBytes(_.arena[off + prefix.len .. off + total], num_buf[0..nlen]);
        _.arena_len += total;
        _.args[_.arg_count] = WorkerArgSpan{ offset: off, len: total };
        _.arg_count += 1;
        return _.arg_count;
    }
}

/// 单个 Worker 进程的环境变量规划（1-indexed `JEST_WORKER_ID` / `BUN_TEST_WORKER_ID`）
pub struct WorkerEnvPlan {
    worker_id_1based: u32 = 1,
    force_color: bool = false,
    needs_tmp_dir: bool = false,
    worker_junit_flag: bool = false,

    static fn forWorker(
        worker_idx_0based: u32,
        enable_ansi_stderr: bool,
        enable_junit: bool,
        enable_coverage: bool,
    ): WorkerEnvPlan {
        return WorkerEnvPlan{
            worker_id_1based: worker_idx_0based + 1,
            force_color: enable_ansi_stderr,
            needs_tmp_dir: enable_junit || enable_coverage,
            worker_junit_flag: enable_junit,
        };
    }

    fn formatWorkerId(out: []u8): usize! {
        if (_.worker_id_1based == 0) return err.InvalidWorkerId;
        let tmp: [16]u8 = [0; 16];
        const n = formatU32Dec(_.worker_id_1based, tmp[0..]);
        if (n > out.len) return err.BufferTooSmall;
        copyBytes(out[0..n], tmp[0..n]);
        return n;
    }
}

// ----------------------------------------------------------------------------
// 内部零分配辅助函数
// ----------------------------------------------------------------------------

fn satSubU32(a: u32, b: u32): u32 {
    if (a <= b) return 0;
    return a - b;
}

fn asciiOrderLess(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) {
            return a[i] < b[i];
        }
        i += 1;
    }
    return a.len < b.len;
}

fn parseSignedI64(raw: []const u8): i64? {
    if (raw.len == 0) return null;
    let idx: usize = 0;
    let neg = false;
    if (raw[0] == '-') {
        neg = true;
        idx = 1;
    } else if (raw[0] == '+') {
        idx = 1;
    }
    if (idx >= raw.len) return null;
    let acc: i64 = 0;
    while (idx < raw.len) {
        const c = raw[idx];
        if (c < '0' || c > '9') return null;
        acc = acc * 10 + @as(i64, c - '0');
        idx += 1;
    }
    return if (neg) -acc else acc;
}

fn formatU32Dec(val: u32, out: []u8): usize {
    if (val == 0) {
        out[0] = '0';
        return 1;
    }
    let tmp: [16]u8 = [0; 16];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = @as(u8, @truncate(v % 10)) + '0';
        v /= 10;
        n += 1;
    }
    let i: usize = 0;
    while (i < n) {
        out[i] = tmp[n - 1 - i];
        i += 1;
    }
    return n;
}

fn copyBytes(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i];
        i += 1;
    }
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    return @as(u32, buf[off]) |
        (@as(u32, buf[off + 1]) << 8) |
        (@as(u32, buf[off + 2]) << 16) |
        (@as(u32, buf[off + 3]) << 24);
}

fn writeU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = @as(u8, @truncate(v));
    buf[off + 1] = @as(u8, @truncate(v >> 8));
    buf[off + 2] = @as(u8, @truncate(v >> 16));
    buf[off + 3] = @as(u8, @truncate(v >> 24));
}
