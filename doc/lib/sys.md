# CP 标准库：跨平台系统调用、文件句柄、元数据与进程控制手册 (`cap:sys`)

## 一、 架构总览与设计准则

`cap:sys` 是 CP 语言官方内置的底层系统调用与操作系统原语标准库，深度吸收 Bun 生产级 `src/sys/` 实现，并完全基于 Zig 0.17 原生 `std.c.*` 与 `std.os.linux.*` 构建（零 `@import("bun")` 与零废弃 `std.posix` 依赖）。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP 系统编程与运行时底座 (`cap:sys`)                 │
├──────────────────────┬─────────────────────────┬────────────────────────┤
│ 结构化错误与结果类型  │  *at 原子文件与零堆目录   │  零拷贝、Poll 与子进程  │
│ Maybe<T>             │  openat / mkdirat       │  copyFileRange         │
│ SysError             │  unlinkat / renameat2   │  sendfile              │
│ SyscallTag           │  symlinkat / readlinkat │  copyFileWithState     │
│ SystemErrno 联动     │  fstatat / lstatat/statx│  poll / epoll*         │
│ EINTR 自动重试       │  DirIterator(getdents64)│  PosixSpawn / waitpid  │
├──────────────────────┴─────────────────────────┴────────────────────────┤
│              Zig 0.17 std.c.* / std.os.linux.* 原生适配层               │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/sys/error.cp` | `// Source: /data/dev/bun/src/sys/Error.cp` | `SyscallTag` 枚举、`SysError`（`fromCode`/`fromErrno`/`withPath`/`withDest`/`withFd`/`toError`/`format`/语义谓词）、`Maybe<T>`（`initResult`/`initErr`/`isOk`/`isErr`/`errno`/`asValue`/`asErr`/`unwrap`/`unwrapOr`）、`linuxSyscallErrno`/`cErrno` |
| `lib/sys/fd.cp` | `// Source: /data/dev/bun/src/sys/fd.cp` | `FD` 强类型描述符（`cwd`/`fromSystem`/`unwrapValid`/`native`/`isTty`/`isNonblocking`/`setNonblocking`/`setCloseOnExec`/`dupCloexec`/`flock`/`fchmod`/`fchown`/`getFdPath`/`readMaybe`/`writeMaybe`/`preadMaybe`/`pwriteMaybe`）、`O_*`/`AT_*`/`LOCK_*`/`F_*` 常量、`flagsToLinuxO` |
| `lib/sys/stat.cp` | `// Source: /data/dev/bun/src/sys/PosixStat.cp & /data/dev/bun/src/sys/sys.cp` | `FileKind`、`StatxField`、`Stat`/`PosixStat`（含纳秒时间戳与 `kind`/`isSymLink`/`isSymlink`），以及 `stat`/`lstat`/`fstat`/`fstatat`/`lstatat`/`statx`（含 `*Maybe` 变体） |
| `lib/sys/dir.cp` | `// Source: /data/dev/bun/src/runtime/node/dir_iterator.cp & /data/dev/bun/src/sys/dir.cp` | 基于 Linux `getdents64` 与 8KB 栈内联缓冲的零堆目录迭代器 `DirIterator`、`DirEntry`、`DirEntryKind` 与 `Dir` |
| `lib/sys/file.cp` | `// Source: /data/dev/bun/src/sys/File.cp` | `File` 高阶句柄（`open`/`create`/`openReadOnly`/`openat`/`openMaybe`/`openatMaybe`/`makeOpen`/`makeOpenat`/`fromFd`/`readAll`/`pwriteAll`/`readToEndAlloc`/`fdatasync`/`flock`/`fchmod`/`setNonblocking`/`isTty`/`getPath`） |
| `lib/sys/copy_file.cp` | `// Source: /data/dev/bun/src/sys/copy_file.cp & /data/dev/bun/src/sys/sys.cp` | `CopyFileState`、`copyFileRange`、`sendfile`、`copyFileWithState`（三级自适应：`copy_file_range` -> `sendfile` -> 32KB R/W）、`copyFileByFd`、`copyFile` |
| `lib/sys/fs_ops.cp` | `// Source: /data/dev/bun/src/sys/sys.cp` | POSIX/Linux 系统调用全集：`openat`/`mkdirat`/`mkdirAll`/`mkdirAllAt`/`unlinkat`/`rmdirat`/`rmdirRecursive`/`renameat`/`renameat2`/`symlinkat`/`readlinkat`/`link`/`linkat`/`fchmod`/`chown`/`fchown`/`fsync`/`fdatasync`/`flock`/`fcntl`/`ioctl`/`createPipe`/`pipe2`/`poll`/`epollCreate`/`epollCtl`/`epollWait`/`exists`/`readFile`/`writeFile` |
| `lib/sys/process.cp` | `// Source: /data/dev/bun/src/runtime/api/bun/spawn.cp & /data/dev/bun/src/platform/linux.cp` | `PosixSpawn`（支持 `cwd`/`envp`/`stdin_fd`/`stdout_fd`/`stderr_fd`/`detached`/`new_process_group`/`pdeathsig`/`search_path` 与子进程 errno 管道回传）、`WaitPidResult`/`waitpid`/`kill`、时钟与硬件自省 |
| `lib/sys/tmp.cp` | `// Source: /data/dev/bun/src/sys/tmp.cp` | `getTempDir` / `makeTempFile` / `makeTempDir` / `removeFile` / `removeDir` |
| `lib/sys/signal.cp` | `// Source: /data/dev/bun/src/sys/SignalCode.cp` | 30 个标准 POSIX `Signal` 枚举、`description`、`toExitCode`、`fromNumber`、`fromName` |
| `lib/sys/group.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /etc/group Parser)` | Linux `/etc/group` 零分配流式解析与成员查询：`GroupEntry`、`GroupMemberIter`、`EtcGroupIter`、`findGroupByName`、`findGroupByGid`、`isUserInGroup` |
| `lib/sys/protocols.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /etc/protocols Parser)` | Linux `/etc/protocols` IP 协议号/别名零分配双向查询：`ProtocolEntry`、`EtcProtocolsIter`、`findProtocolNumber`、`findProtocolByNumber` |
| `lib/sys/services.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /etc/services Parser)` | Linux `/etc/services` 网络服务名与端口/协议零分配双向查询：`ServiceEntry`、`EtcServicesIter`、`findServicePort`、`findServiceByPort` |
| `lib/sys/shells.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /etc/shells Parser)` | Linux `/etc/shells` 合法登录 Shell 路径清单零分配解析与安全校验：`parseShellLine`、`EtcShellsIter`、`isAllowedShell`、`findShellByBasename`、`countShells`、`isNologinShell` |
| `lib/sys/cmdline.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (POSIX NUL-Delimited cmdline & environ Parser)` | `/proc/<pid>/cmdline` 与 `environ` NUL 分隔解析：`NulArgIter`、`countArgs`、`argAt`、`ProcEnvEntry`、`ProcEnvIter`、`findProcEnv`、`formatCmdlineDisplay` |
| `lib/sys/cpuinfo.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/cpuinfo Zero-Alloc Parser)` | `/proc/cpuinfo` 零分配解析：`CpuBlock`、`CpuSummary`、`ProcCpuInfoIter`、`summarizeCpuInfo`、`hasCpuFlag` |
| `lib/sys/procio.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/<pid>/io Parser)` | `/proc/<pid>/io` 进程 I/O 统计与窗口增量：`ProcIoStats`、`parseProcIo` |
| `lib/sys/limits.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/<pid>/limits Zero-Alloc Parser)` | `/proc/<pid>/limits` 资源限制解析：`LimitValue`、`ProcLimitEntry`、`ProcLimitsIter`、`parseLimitValue`、`findProcLimit`、`maxOpenFiles` |
| `lib/sys/loadavg.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/loadavg & /proc/uptime Barrel)` | `/proc/loadavg` 负载均值解析：`LoadAvg`、`SystemUptime` |
| `lib/sys/meminfo.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/meminfo Barrel)` | `/proc/meminfo` 系统内存统计解析：`MemInfoField`、`MemInfoIter`、`MemInfo` |
| `lib/sys/mounts.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/mounts Zero-Alloc Parser)` | `/proc/mounts` 挂载表与八进制反转义：`MountEntry`、`ProcMountsIter`、`unescapeMountPath`、`findMountForPath` |
| `lib/sys/netdev.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/net/dev Network Interface Stats Parser)` | `/proc/net/dev` 网卡流量计数解析：`NetDevStats`、`ProcNetDevIter`、`findNetDev`、`sumNonLoopbackBytes` |
| `lib/sys/route.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/net/route Routing Table Parser)` | `/proc/net/route` 路由表与默认网关解析：`RTF_UP`、`RTF_GATEWAY`、`RTF_HOST`、`parseHexIpv4Le`、`RouteEntry`、`ProcRouteIter`、`findDefaultGateway` |
| `lib/sys/procstatus.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/<pid>/status Zero-Alloc Parser)` | `/proc/<pid>/status` 进程状态与内存/线程字段：`ProcStatus`、`parseProcStatus`、`findProcStatusField` |
| `lib/sys/procstat.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/<pid>/stat Barrel)` | `/proc/<pid>/stat` 单行进程状态解析：`ProcState`、`parseProcState`、`ProcStat` |
| `lib/sys/statm.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/<pid>/statm Parser)` | `/proc/<pid>/statm` 7 列内存页与饱和换算：`ProcStatm`、`parseProcStatm` |
| `lib/sys/uptime.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/uptime Zero-Alloc Integer Parser)` | `/proc/uptime` 零浮点整数运行时间解析：`ProcUptime`、`parseProcUptime` |
| `lib/sys/index.cp` | `// Source: /data/dev/bun/src/sys/sys.zig & /data/dev/bun/src/sys/Error.cp` | `cap:sys` 统一导出、`Sys`/`sys` 命名空间（含 `Sys.pipe`/`Sys.pipe2`/`Sys.dup`/`Sys.dup2`/`Sys.setAffinity`/`Sys.Lib`） |

---

## 二、 核心 API 矩阵

### 1. 结构化错误与零异常开销分支 (`Maybe<T>` / `SysError`)

- `Maybe<T>`：`Maybe<T>.initResult(val)`、`Maybe<T>.initErr(se)`、`isOk()`、`isErr()`、`errno()`、`asValue()`、`asErr()`、`unwrap()`、`unwrapOr(default)`。
- `SysError`：`fromCode(code, tag)`、`fromErrno(e, tag)`、`withPath(p)`、`withDest(d)`、`withFd(fd)`、`toError()`、`format(buf)`、`isRetry()`、`isNotFound()`、`isAlreadyExists()`。

### 2. `*at` 原子目录/文件原语与 `DirIterator`

- `mkdirAll(path, mode)` / `mkdirAllAt(dir_fd, path, mode)`、`rmdirRecursive(path)` / `rmdirRecursiveAt(dir_fd, path)`
- `openat`/`openatMaybe`、`fstatat`/`lstatat`/`statx`、`symlinkat`/`readlinkat`、`renameat2`（`RENAME_NOREPLACE` / `RENAME_EXCHANGE`）
- `DirIterator.open(path)` / `DirIterator.openAt(dir_fd, path)`：`next()` 返回 `DirEntry?`，零堆内存扫描目录项与文件类型。

### 3. 子进程派生 (`PosixSpawn`)

- `SpawnOptions`：`cwd`、`envp`、`stdin_fd`、`stdout_fd`、`stderr_fd`、`detached`、`new_process_group`、`pdeathsig`、`search_path`。
- `PosixSpawn.spawn(allocator, argv, opts): !i32` 与 `PosixSpawn.spawnMaybe(...)`: `Maybe<i32>`。
- `waitpid(pid, flags): !WaitPidResult`（`exited()` / `exitCode()` / `signaled()` / `termSig()` / `stopped()`）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    FD, DirIterator, PosixSpawn, SpawnOptions,
    O_RDONLY, O_RDWR, O_CREAT, O_TRUNC, O_CLOEXEC,
    AT_FDCWD, openat, mkdirAll, rmdirRecursive, waitpid,
} from "cap:sys";

pub fn demoPhaseASys(): !void {
    const gpa = std.heap.page_allocator;
    try mkdirAll("/tmp/cp_sys_demo/sub", 0o755);
    defer * = rmdirRecursive("/tmp/cp_sys_demo") catch {};

    const fd = try openat(FD.fromNative(AT_FDCWD), "/tmp/cp_sys_demo/sub/hello.txt", O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, 0o644);
    try fd.writeAll("hello cap:sys");
    fd.close();

    let it = try DirIterator.open("/tmp/cp_sys_demo/sub");
    defer it.close();
    while (true) {
        const entry = try it.next();
        if (entry == null) break;
        * = entry.?.name;
    }

    let argv: [2][]const u8 = ["/bin/echo", "spawn-ok"];
    const pid = try PosixSpawn.spawn(gpa, argv[0..2], SpawnOptions{ cwd: "/tmp", search_path: true });
    const res = try waitpid(pid, 0);
    * = res.exited();
}
```
