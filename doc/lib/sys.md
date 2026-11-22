# CP 标准库：跨平台系统调用、文件句柄、元数据与进程控制手册 (`cap:sys`)

> 参考来源：`/data/dev/bun/src/sys/sys.zig`, `/data/dev/bun/src/sys/`, `/data/dev/bun/docs/runtime/file-io.mdx` | 吸收日期：2026-11-22

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
| `lib/sys/cgroup.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux cgroup v2 Resource Limits Parser)` | cgroup v2 资源限额解析：`CgroupLimit`、`CpuMaxQuota`、`IoMaxEntry`、`parseCgroupMax`、`parseCpuMax`、`parseIoMaxLine`、`effectiveMemoryLimit` |
| `lib/sys/sysctl.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux sysctl.conf Zero-Alloc Parser)` | `/etc/sysctl.conf` 配置解析与键归一化：`SysctlEntry`、`SysctlIter`、`normalizeSysctlKey`、`sysctlKeysEqual`、`getSysctlValue` |
| `lib/sys/fstab.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (POSIX /etc/fstab & /proc/mounts Parser)` | `/etc/fstab` 挂载表解析与选项查询：`FstabEntry`、`MountIter`、`unescapeFstabPath`、`findFstabForPath` |
| `lib/sys/osrelease.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (freedesktop.org /etc/os-release Barrel)` | `/etc/os-release` 发行版标识解析：`OsReleaseField`、`OsReleaseIter`、`getOsReleaseField`、`isOsLike` |
| `lib/sys/passwd.cp` | `// Source: Absorbed from /data/dev/bun/src/sys.zig (POSIX /etc/passwd & /etc/group Barrel)` | `/etc/passwd` 与组查询：`PasswdEntry`、`PasswdIter`、`lookupUserByName`、`lookupUserByUid`、`PasswdGroupEntry`、`GroupIter`、`lookupGroupByName`、`lookupGroupByGid` |
| `lib/sys/resolv.cp` | `// Source: Absorbed from /data/dev/bun/src/dns.zig (POSIX /etc/resolv.conf Barrel)` | `/etc/resolv.conf` DNS 解析配置：`NameserverIter`、`ResolvOptions`、`effectiveSearchRaw`、`firstNameserver` |
| `lib/sys/systemd.cp` | `// Source: Absorbed from /data/dev/bun/src/ini.zig (Linux systemd Unit File Parser)` | systemd unit 文件解析：`UnitEntry`、`UnitIter`、`getUnitValue`、`countSectionKey` |
| `lib/sys/xdg.cp` | `// Source: Absorbed from /data/dev/bun/src/env_loader.zig (XDG Base Directory Spec Resolver)` | XDG Base Directory 规范路径解析：`DEFAULT_CONFIG_DIRS`、`DEFAULT_DATA_DIRS`、`XdgEnv`、`XdgDirIter`、`resolveConfigHome`/`DataHome`/`CacheHome`/`StateHome`/`RuntimeDir` |
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

---

## 上游 Bun `sys` 行为契约与系统调用安全边界

1. **`*at` 家族原子操作与 `O_CLOEXEC` 默认防泄漏**：
   - 继承自 Bun `src/sys/`：打开文件与目录操作优先使用 `openat` / `mkdirat` / `unlinkat` / `renameat2` / `fstatat` 并附加 `O_CLOEXEC`（或 `FD.dupCloexec`），避免 TOCTOU 路径竞态以及子进程 `PosixSpawn` 意外继承父进程文件描述符。
2. **`EINTR` 自动重试与 `Maybe<T>` 结构化错误**：
   - 阻塞型系统调用（读写、`waitpid`、`poll` 等）内部自动重试 `EINTR` 信号中断；`Maybe<T>` 完整保留原始操作系统 `errno`、触发调用 `SyscallTag` 与关联路径，便于精确区分 `ENOENT`、`EACCES`、`EEXIST`、`EXDEV`（跨文件系统重命名回退拷贝）与 `EAGAIN`。
3. **内核零拷贝文件复制回退链（`copyFileRange` / `sendfile`）**：
   - `copyFileWithState` 优先尝试 Linux `copy_file_range` / `sendfile` 内核零拷贝路径，在目标文件系统返回 `EXDEV` / `ENOSYS` / `EINVAL` 时平滑回退至用户态缓冲读写。
4. **`DirIterator` 零堆 `getdents64` 遍历**：
   - 使用 8KB 栈内联缓冲批量读取内核目录项，调用方须配对 `defer it.close();` 并在遍历期间消费当前 `entry.name` 切片（下次 `next()` 会复用同一缓冲）。

---

## 收敛集成的内核/固件、文件系统、虚拟磁盘与无锁缓冲套件

以下纯 CP 零外部依赖系统底层模块已统一聚合导出至 `cap:sys`（`lib/sys/index.cp`），零符号冲突：

| 分类 | 文件路径 | 规范 / 格式 | 说明 |
| :--- | :--- | :--- | :--- |
| 内核、固件与总线 | `lib/sys/bpf.cp` | RFC 9669 eBPF | 64-bit/128-bit eBPF 指令编解码与程序校验器 |
| 内核、固件与总线 | `lib/sys/acpi.cp` | ACPI 6.5 | RSDP / SDT / MADT / MCFG 固件表零堆分配解析器 |
| 内核、固件与总线 | `lib/sys/smbios.cp` | SMBIOS / DMI | 固件 SMBIOS 结构表解析器 |
| 内核、固件与总线 | `lib/sys/fdt.cp` | Devicetree Spec v0.4 | Flattened Device Tree (`.dtb`) 解析器 |
| 内核、固件与总线 | `lib/sys/pci.cp` | PCI / PCIe | 配置空间头与 Capability 链表解析器 |
| 内核、固件与总线 | `lib/sys/hid.cp` | USB HID 1.11 | HID 报告描述符解析与摘要器 |
| 内核、固件与总线 | `lib/sys/edid.cp` | VESA EDID 1.3 / 1.4 | 显示器 EDID 块与时序解析器 |
| 文件系统与分区 | `lib/sys/gpt.cp` | UEFI GPT / Protective MBR | GPT 头、分区条目解析与 CRC32 校验 |
| 文件系统与分区 | `lib/sys/ext4.cp` | Linux ext2 / ext3 / ext4 | 超级块、组描述符、Inode 与目录解析器 |
| 文件系统与分区 | `lib/sys/btrfs.cp` | Linux Btrfs | 超级块、Chunk 树与元数据解析器 |
| 文件系统与分区 | `lib/sys/xfs.cp` | Linux XFS | XFS 超级块与 Inode 解析器（常量别名 `XFS_S_IFMT`/`XFS_S_IFREG`/`XFS_S_IFDIR`） |
| 文件系统与分区 | `lib/sys/zfs.cp` | OpenZFS | Vdev Label、Uberblock 与 NVList 解析器 |
| 文件系统与分区 | `lib/sys/fat.cp` | FAT12 / FAT16 / FAT32 | BPB 与目录条目解析器 |
| 文件系统与分区 | `lib/sys/exfat.cp` | Microsoft exFAT | 引导扇区、校验和与 32B 目录项解析器（常量别名 `EXFAT_ATTR_*`） |
| 文件系统与分区 | `lib/sys/ntfs.cp` | Microsoft NTFS | 引导扇区与 MFT 记录解析器 |
| 文件系统与分区 | `lib/sys/apfs.cp` | Apple APFS | 容器/卷超级块与 Fletcher-64 校验解析器 |
| 虚拟磁盘与块存储 | `lib/sys/qcow2.cp` / `lib/sys/vhdx.cp` | QEMU QCOW2 v2/v3 & VHDX | 虚拟磁盘镜像头与映射表解析器 |
| 虚拟磁盘与块存储 | `lib/sys/scsi.cp` | SCSI SPC / SBC | SCSI CDB、Inquiry 与 Sense 数据编解码 |
| 并发缓冲 | `lib/sys/ringbuf.cp` | RingBuffer / ByteRing / MicroBatcher | 零拷贝环形缓冲与微批聚合器 |
| 服务编排与架构（Batch 40） | `lib/sys/di.cp` | Dependency Injection & IoC | `ServiceContainer`、`ServiceScope`、Singleton/Scoped/Transient 生命周期与循环依赖检测 |
| 服务编排与架构（Batch 40） | `lib/sys/pipeline.cp` | Onion Middleware Pipeline | 可组合洋葱中间件与多阶段流水线 (`Pipeline`, `PipelineContext`, `StageSpec`) |
| 服务编排与架构（Batch 40） | `lib/sys/pubsub_topic.cp` / `lib/sys/pubsub_broker.cp` | NATS / MQTT Wildcard Pub/Sub | 进程内通配符发布订阅代理、背压丢弃策略与 Retained 消息重放 |
| 并发与批量通道 | `lib/sys/byte_ring.cp` | 2-Pow Bitmask Byte/Frame Ring | `ByteRingQueue` / `ByteRingStats`：零堆分配 2 的幂位掩码批量环形字节队列与 4B 小端长度前缀帧原语 |
| 句柄池与资源表 | `lib/sys/gen_slab.cp` | ABA-Safe Generational Slab | `GenSlabPool` / `GenHandle` / `GenSlabStats`：零堆分配 O(1) 代次安全定长对象句柄池（防 ABA 与 double-free） |

---

### 零堆分配 2 的幂掩码批量环形字节/帧队列示例 (`byte_ring.cp`)

```cp
import {
    ByteRingQueue,
} from "cap:sys";

pub fn demoByteRingQueue() {
    let storage: [64]u8 = [0; 64]; // 必须为 >= 2 的 2 的幂次
    let ring = ByteRingQueue.init(storage[0..]) ?? return;

    // 1. 4 字节小端长度前缀原子帧写入/读取（零粘包）
    * = ring.writeFrame("agent-event-1");
    * = ring.writeFrame("agent-event-2");

    let frame_buf: [32]u8 = [0; 32];
    if (n1 := ring.readFrame(frame_buf[0..])) {
        * = frame_buf[0..n1]; // "agent-event-1"
    }
    if (n2 := ring.readFrame(frame_buf[0..])) {
        * = frame_buf[0..n2]; // "agent-event-2"
    }
    * = ring.stats();
}
```

### 零堆分配代次安全（Generational）Slab 对象句柄池示例 (`gen_slab.cp`)

面向 Actor 运行时、连接句柄表与无 GC 资源注册表的 100% 纯 CP 零动态堆分配 O(1) 代次安全定长句柄池（最高 256 槽位，LIFO 空闲栈 + Bump 游标 + 非零 32 位代次防 ABA / Double-Free）：

- **导出符号**：`GEN_SLAB_MAX_SLOTS` (`256`)、`GEN_SLAB_NO_FREE` (`0xFFFF`)、`GenHandle`（`index: u16`, `generation: u32`, `toU64()`, `fromU64(raw)`, `eql(other)`）、`GenSlabStats`、`GenSlabPool`（`init`, `alloc`, `get`, `set`, `free`, `is_alive`, `clear`, `stats`）。

```cp
import {
    GenHandle,
    GenSlabPool,
} from "cap:sys";

pub fn demoGenSlabPool() {
    let pool = GenSlabPool.init(64) ?? return;

    const h1 = pool.alloc(0xCAFE_0001) ?? return;
    const raw_id = h1.toU64();
    const decoded = GenHandle.fromU64(raw_id);
    * = pool.set(decoded, 0xCAFE_0002);

    // 释放 h1 后复用同一槽位 -> 新句柄 h2 代次递增，旧句柄 h1 严格失效（防 ABA）
    * = pool.free(h1);
    const h2 = pool.alloc(0xBEEF_0003) ?? return;
    * = pool.is_alive(h1); // false
    * = pool.is_alive(h2); // true
    * = pool.stats();
}
```

## 收敛子模块深度文档

- [acpi.md](sys/acpi.md)
- [apfs.md](sys/apfs.md)
- [bpf.md](sys/bpf.md)
- [btrfs.md](sys/btrfs.md)
- [di.md](sys/di.md)
- [edid.md](sys/edid.md)
- [exfat.md](sys/exfat.md)
- [ext4.md](sys/ext4.md)
- [fat.md](sys/fat.md)
- [fdt.md](sys/fdt.md)
- [fs.md](sys/fs.md)
- [gpt.md](sys/gpt.md)
- [hid.md](sys/hid.md)
- [ntfs.md](sys/ntfs.md)
- [pci.md](sys/pci.md)
- [pipeline.md](sys/pipeline.md)
- [pubsub.md](sys/pubsub.md)
- [qcow2.md](sys/qcow2.md)
- [ringbuf.md](sys/ringbuf.md)
- [scsi.md](sys/scsi.md)
- [smbios.md](sys/smbios.md)
- [vhdx.md](sys/vhdx.md)
- [xfs.md](sys/xfs.md)
- [zfs.md](sys/zfs.md)

