# CP 标准库：异步 I/O 队列与父进程看门狗子系统使用手册 (`cap:aio`)

## 一、 架构总览与设计哲学

`cap:aio` 是 CP 语言官方内置的**Linux 父进程退出看门狗（`ParentDeathWatchdog`）与零堆分配 POSIX 异步提交/完成环形队列（`AioQueue<CAP>`）**标准库。吸收自 Bun（`/data/dev/bun/src/aio/`），全部基于 Zig 0.17 原生 `std.os.linux.*` 与 `std.c.*` 构建，为多进程监督防护与批量文件 I/O 调度提供零堆分配、确定性的底层基础设施。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              cap:aio 异步 I/O 与进程守护防线                           │
├───────────────────────────────────────────┬────────────────────────────────────────────┤
│    父进程看门狗 (watchdog.cp)              │     零堆提交/完成环 (queue.cp)               │
│  • ParentDeathWatchdog.init()             │  • AioOpKind (Open/Read/Write/Pread/Stat)  │
│  • install(sig) (prctl PR_SET_PDEATHSIG)  │  • AioState (Pending/Submitted/Completed)  │
│  • isParentAlive() (PPID 变更 + kill 0)   │  • AioQueue<CAP> (submit/cancel/drain)     │
│  • parentPidOf(pid) (/proc/<pid>/stat)    │  • AioCompletion (bytes/fd/errno/stat)     │
└───────────────────────────────────────────┴────────────────────────────────────────────┘
```

---

## 二、 核心组件详解与代码示例

### 1. 父进程死亡看门狗 (`ParentDeathWatchdog`)

当子进程需要防止在父进程被意外终止（如 `SIGKILL`）后成为孤儿进程驻留时，通过 `ParentDeathWatchdog` 捕获初始 PPID 并向 Linux 内核注册 `PR_SET_PDEATHSIG`：

- `install(sig)`：调用 `std.os.linux.prctl(PR.SET_PDEATHSIG, sig, ...)` 并在调用后二次核验 `getppid()` 防止窗口期竞态；
- `isParentAlive()`：检测 `getppid()` 是否发生重挂（Reparenting）以及 `kill(ppid, 0)` 探测父进程是否存活；
- `clear()`：解除 `PR_SET_PDEATHSIG` 绑定。

```typescript
import { ParentDeathWatchdog } from "cap:aio";

fn armWatchdog(): ! {
    let wd = ParentDeathWatchdog.init();
    const armed = try wd.install(15); // SIGTERM = 15
    if (armed && wd.isParentAlive()) {
        const ppid = wd.parentPid();
        * = ppid;
    }
    try wd.clear();
}
```

### 2. 零堆分配提交/完成队列 (`AioQueue<const CAP: usize>`)

`AioQueue<CAP>` 在栈或结构体内部维护固定容量的提交环（SQ）与完成环（CQ），支持：

- `submit(op)` / `submitBatch(ops)`：入队并自动分配单调递增操作 ID；
- `cancel(op_id)`：在执行前取消指定操作，消费时生成 `AioState.Cancelled` 完成项（`errno_code = 125`）；
- `flush()` / `poll(out)` / `drain(out)`：按 FIFO 顺序真实执行 POSIX/Linux `open`、`read`、`write`、`pread`、`pwrite`、`statx`、`fsync`、`close`、`unlink`、`mkdir`，返回结构化 `AioCompletion`。

```typescript
import {
    AioQueue,
    AioOp,
    AioCompletion,
    AIO_O_RDWR,
    AIO_O_CREAT,
    AIO_O_TRUNC,
} from "cap:aio";

fn batchFileIo(): ! {
    let q = AioQueue<16>.init();
    let comps: [16]AioCompletion = undefined;

    * = try q.submit(AioOp.open("temp/aio.txt", AIO_O_RDWR | AIO_O_CREAT | AIO_O_TRUNC, 0o644));
    * = q.drain(&comps);
    const fd = comps[0].result_fd;

    const ops = [
        AioOp.write(fd, "hello "),
        AioOp.pwrite(fd, "world", 6),
        AioOp.fsync(fd),
        AioOp.close(fd),
    ];
    * = try q.submitBatch(&ops);
    const done = q.drain(&comps);
    * = done;
}
```

---

## 三、 与 Bun (`src/aio/`) 对照表

| 核心能力 | Bun (`src/aio/*.zig`) | CP 标准库 (`cap:aio`) |
| :--- | :--- | :--- |
| **Linux 父进程看门狗** | `ParentDeathWatchdog` (`PR_SET_PDEATHSIG`) | `ParentDeathWatchdog` (`install`, `clear`, `isParentAlive`, `parentPidOf`) |
| **批量 I/O 队列与状态机** | `posix_event_loop.zig` | `AioQueue<CAP>`, `AioOp`, `AioCompletion`, `AioOpKind`, `AioState` |
