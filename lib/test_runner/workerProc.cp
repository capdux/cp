// Source: Absorbed from /data/dev/bun/src/cli/test/parallel/Worker.cp | Synced: 2026-11-22T14:54:00Z
// Pure-CP zero-allocation Parallel Test Runner Worker Process State Machine:
// - Models a single isolated test worker slot with stdout/stderr pipe EOF tracking,
//   atomic per-file output capture buffer (zero interleaving across concurrent files),
//   two-barrier reap gating (`exit_status != null && ipc_done`),
//   corrupt-frame kill-and-reap policy, atomic start rollback (EMFILE recursion guard),
//   and POSIX/Windows spawn isolation policy descriptors.

/// 单个 Worker 进程每文件内联捕获缓冲区容量（stdout + stderr 合并，零堆分配）
pub const WORKER_PROC_CAPTURE_CAP: usize = 2048;

/// Worker 进程状态机错误集
pub err WorkerProcError {
    AlreadyAlive,
    NotAlive,
    AlreadyInflight,
    NotStarting,
    InvalidState,
    BufferTooSmall,
}

/// Worker 管道角色（对齐 Bun `WorkerPipeRole`）
pub enum WorkerPipeRole: u8 {
    Stdout = 0,
    Stderr = 1,
}

/// Worker 进程退出状态分类
pub enum WorkerExitKind: u8 {
    Exited = 0,
    Signaled = 1,
    Unknown = 2,
}

/// Worker 进程退出状态快照
pub struct WorkerExitStatus {
    kind: WorkerExitKind = .Exited,
    code: i32 = 0,

    static fn exited(code: i32): WorkerExitStatus {
        return WorkerExitStatus{ kind: .Exited, code: code };
    }

    static fn signaled(sig: i32): WorkerExitStatus {
        return WorkerExitStatus{ kind: .Signaled, code: sig };
    }

    fn isCleanSuccess(): bool {
        return _.kind == .Exited && _.code == 0;
    }
}

/// Worker 生命周期阶段
pub enum WorkerLifecyclePhase: u8 {
    Idle = 0,
    Starting = 1,
    Running = 2,
    ShuttingDown = 3,
    AwaitingReap = 4,
    Reaped = 5,
}

/// `onChannelDone()` 返回的宿主调度动作
pub enum WorkerIpcAction: u8 {
    /// 进程仍在退出中或已优雅关闭，等待双门槛满足后调用 `tryReap()`
    WaitForExit = 0,
    /// 畸形帧路径：IPC 仍处于 attached 状态，需强制终止子进程并回收
    KillAndReap = 1,
    /// 进程已退出且 IPC 已结束，立即执行槽位回收
    ReadyToReap = 2,
}

/// 跨平台 Worker 子进程隔离启动策略（对齐 Bun `Worker.start` POSIX/Windows 分支）
pub struct WorkerSpawnPolicy {
    is_windows: bool = false,
    ipc_fd: i32 = 3,
    new_process_group: bool = true,
    use_linux_pdeathsig: bool = true,
    use_windows_job_object: bool = false,
    use_unix_socketpair: bool = true,

    static fn forPosix(is_linux: bool): WorkerSpawnPolicy {
        return WorkerSpawnPolicy{
            is_windows: false,
            ipc_fd: 3,
            new_process_group: true,
            use_linux_pdeathsig: is_linux,
            use_windows_job_object: false,
            use_unix_socketpair: true,
        };
    }

    static fn forWindows(): WorkerSpawnPolicy {
        return WorkerSpawnPolicy{
            is_windows: true,
            ipc_fd: 3,
            new_process_group: false,
            use_linux_pdeathsig: false,
            use_windows_job_object: true,
            use_unix_socketpair: false,
        };
    }
}

/// 单个并行测试 Worker 进程槽位状态机（对齐 Bun `cli/test/parallel/Worker.cp`）
pub struct WorkerProcessSlot {
    idx: u32 = 0,
    phase: WorkerLifecyclePhase = .Idle,
    alive: bool = false,
    /// 当前正在该 Worker 上执行的测试文件索引
    inflight_file: u32? = null,
    /// 最近一次派发时间戳（毫秒），驱动协调器懒扩容判定
    dispatched_at_ms: i64 = 0,
    /// 本次运行已完成的测试文件计数
    completed_files: u32 = 0,

    /// stdout / stderr / ipc 三条通道完成标记
    stdout_done: bool = false,
    stderr_done: bool = false,
    ipc_done: bool = false,
    ipc_attached: bool = false,

    /// 进程退出状态；与 `ipc_done` 构成双门槛回收条件
    exit_status: WorkerExitStatus? = null,

    /// 自上次 `file_done` 以来捕获的 stdout+stderr 合并缓冲
    captured: [WORKER_PROC_CAPTURE_CAP]u8 = [0; WORKER_PROC_CAPTURE_CAP],
    captured_len: usize = 0,
    truncated_bytes: usize = 0,

    static fn init(idx: u32): WorkerProcessSlot {
        return WorkerProcessSlot{ idx: idx };
    }

    /// 进入原子启动事务（对应 `WorkerStartGuard`）
    fn beginStart(*_): WorkerProcError!bool {
        if (_.alive || _.phase == .Starting) {
            return err.AlreadyAlive;
        }
        _.phase = .Starting;
        _.stdout_done = false;
        _.stderr_done = false;
        _.ipc_done = false;
        _.ipc_attached = false;
        _.exit_status = null;
        _.inflight_file = null;
        _.captured_len = 0;
        _.truncated_bytes = 0;
        return true;
    }

    /// 标记进程与 IPC 通道启动成功（解除 `WorkerStartGuard` 回滚）
    fn commitStart(*_, ipc_attached: bool): WorkerProcError!bool {
        if (_.phase != .Starting) {
            return err.NotStarting;
        }
        _.alive = true;
        _.ipc_attached = ipc_attached;
        _.ipc_done = !ipc_attached;
        _.phase = .Running;
        return true;
    }

    /// 启动/watchOrReap 失败时的原子回滚（不触发 `onProcessExit`，防止 EMFILE 递归风暴）
    fn abortStart(*_) {
        _.alive = false;
        _.phase = .Idle;
        _.inflight_file = null;
        _.ipc_attached = false;
        _.ipc_done = true;
        _.stdout_done = true;
        _.stderr_done = true;
        _.exit_status = null;
        _.captured_len = 0;
        _.truncated_bytes = 0;
    }

    /// 派发单个测试文件至本 Worker（对齐 `Worker.dispatch`）
    fn dispatchFile(*_, file_idx: u32, now_ms: i64): WorkerProcError!u32 {
        if (!_.alive || _.phase != .Running) {
            return err.NotAlive;
        }
        if (_.inflight_file != null) {
            return err.AlreadyInflight;
        }
        _.inflight_file = file_idx;
        _.dispatched_at_ms = now_ms;
        return file_idx;
    }

    /// 标记当前在途测试文件完成并清空在途槽位
    fn completeInflight(*_): u32? {
        const finished = _.inflight_file;
        _.inflight_file = null;
        if (finished != null) {
            _.completed_files += 1;
        }
        return finished;
    }

    /// 请求优雅关闭（保持 IPC 读端开启以排空尾部 JUnit/Coverage 帧，对齐 `Worker.shutdown`）
    fn requestShutdown(*_): WorkerProcError!bool {
        if (!_.alive) {
            return err.NotAlive;
        }
        _.phase = .ShuttingDown;
        return true;
    }

    /// 追加来自 stdout 或 stderr 管道的输出块至原子捕获缓冲
    fn onPipeChunk(*_, role: WorkerPipeRole, chunk: []const u8): usize {
        * = role;
        if (chunk.len == 0) return 0;
        const avail = WORKER_PROC_CAPTURE_CAP - _.captured_len;
        const to_copy = if (chunk.len <= avail) chunk.len else avail;
        if (to_copy > 0) {
            copyBytes(_.captured[_.captured_len .. _.captured_len + to_copy], chunk[0..to_copy]);
            _.captured_len += to_copy;
        }
        if (chunk.len > to_copy) {
            _.truncated_bytes += chunk.len - to_copy;
        }
        return to_copy;
    }

    /// 标记指定管道（stdout 或 stderr）遇到 EOF 或读取错误
    fn onPipeDone(*_, role: WorkerPipeRole) {
        switch (role) {
            .Stdout => _.stdout_done = true,
            .Stderr => _.stderr_done = true,
        }
    }

    fn capturedSlice(): []const u8 {
        return _.captured[0.._.captured_len];
    }

    /// 原子拷贝并清空当前捕获输出，供协调器在对应文件头下一次性输出
    fn takeCaptured(*_, out: []u8): usize! {
        if (_.captured_len > out.len) {
            return err.BufferTooSmall;
        }
        const n = _.captured_len;
        copyBytes(out[0..n], _.captured[0..n]);
        _.captured_len = 0;
        _.truncated_bytes = 0;
        return n;
    }

    /// 进程退出回调（对齐 `Worker.onProcessExit`）；仅记录退出状态，等待 `ipc_done` 才回收
    fn onProcessExit(*_, status: WorkerExitStatus): bool {
        _.alive = false;
        _.exit_status = status;
        _.phase = .AwaitingReap;
        return _.canReap();
    }

    /// IPC 通道结束回调（对齐 `Worker.onChannelDone`）：
    /// 若 `still_attached == true` 说明触发了畸形帧错误，需强制终止子进程以保护在途文件记账
    fn onChannelDone(*_, still_attached: bool): WorkerIpcAction {
        _.ipc_done = true;
        _.ipc_attached = false;
        if (still_attached && _.alive) {
            return .KillAndReap;
        }
        if (_.canReap()) {
            return .ReadyToReap;
        }
        return .WaitForExit;
    }

    /// 双门槛回收判定：进程已退出 且 IPC 通道已耗尽关闭
    fn canReap(): bool {
        return _.exit_status != null && _.ipc_done;
    }

    /// 执行最终槽位回收并复位运行态
    fn reap(*_): bool {
        if (!_.canReap()) {
            return false;
        }
        _.phase = .Reaped;
        _.alive = false;
        _.inflight_file = null;
        return true;
    }
}

/// 按 Bun `concurrentTestGlob` 语义判定测试文件路径是否匹配并发执行通配模式
/// 支持 `*`（单层任意字符，不跨 `/`）、`**`（跨目录任意层）与 `?`（单字符）
pub fn matchesConcurrentTestGlob(pattern: []const u8, path: []const u8): bool {
    if (pattern.len == 0) return false;
    return globMatchImpl(pattern, 0, path, 0);
}

// ----------------------------------------------------------------------------
// 内部零分配辅助函数
// ----------------------------------------------------------------------------

fn copyBytes(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i];
        i += 1;
    }
}

fn globMatchImpl(pat: []const u8, pi_init: usize, str: []const u8, si_init: usize): bool {
    let pi = pi_init;
    let si = si_init;

    while (pi < pat.len) {
        // 识别 `**/` 或结尾 `**`
        if (pi + 1 < pat.len && pat[pi] == '*' && pat[pi + 1] == '*') {
            let next_pi = pi + 2;
            if (next_pi < pat.len && pat[next_pi] == '/') {
                next_pi += 1;
            }
            if (next_pi >= pat.len) {
                return true;
            }
            let k = si;
            while (k <= str.len) {
                if (globMatchImpl(pat, next_pi, str, k)) {
                    return true;
                }
                k += 1;
            }
            return false;
        }

        if (pat[pi] == '*') {
            const next_pi = pi + 1;
            let k = si;
            while (k <= str.len) {
                if (globMatchImpl(pat, next_pi, str, k)) {
                    return true;
                }
                if (k < str.len && str[k] == '/') {
                    break;
                }
                k += 1;
            }
            return false;
        }

        if (si >= str.len) {
            return false;
        }

        if (pat[pi] == '?') {
            if (str[si] == '/') return false;
            pi += 1;
            si += 1;
            continue;
        }

        if (pat[pi] != str[si]) {
            return false;
        }
        pi += 1;
        si += 1;
    }

    return si == str.len;
}
