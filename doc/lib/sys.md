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
| `lib/sys/copyFile.cp` | `// Source: /data/dev/bun/src/sys/copy_file.cp & /data/dev/bun/src/sys/sys.cp` | `CopyFileState`、`copyFileRange`、`sendfile`、`copyFileWithState`（三级自适应：`copy_file_range` -> `sendfile` -> 32KB R/W）、`copyFileByFd`、`copyFile` |
| `lib/sys/fsOps.cp` | `// Source: /data/dev/bun/src/sys/sys.cp` | POSIX/Linux 系统调用全集：`openat`/`mkdirat`/`mkdirAll`/`mkdirAllAt`/`unlinkat`/`rmdirat`/`rmdirRecursive`/`renameat`/`renameat2`/`symlinkat`/`readlinkat`/`link`/`linkat`/`fchmod`/`chown`/`fchown`/`fsync`/`fdatasync`/`flock`/`fcntl`/`ioctl`/`createPipe`/`pipe2`/`poll`/`epollCreate`/`epollCtl`/`epollWait`/`exists`/`readFile`/`writeFile` |
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
| 服务编排与架构（Batch 40） | `lib/sys/pubsubTopic.cp` / `lib/sys/pubsubBroker.cp` | NATS / MQTT Wildcard Pub/Sub | 进程内通配符发布订阅代理、背压丢弃策略与 Retained 消息重放 |
| 并发与批量通道 | `lib/sys/byteRing.cp` | 2-Pow Bitmask Byte/Frame Ring | `ByteRingQueue` / `ByteRingStats`：零堆分配 2 的幂位掩码批量环形字节队列与 4B 小端长度前缀帧原语 |
| 句柄池与资源表 | `lib/sys/genSlab.cp` | ABA-Safe Generational Slab | `GenSlabPool` / `GenHandle` / `GenSlabStats`：零堆分配 O(1) 代次安全定长对象句柄池（防 ABA 与 double-free） |

---

### 零堆分配 2 的幂掩码批量环形字节/帧队列示例 (`byteRing.cp`)

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

### 零堆分配代次安全（Generational）Slab 对象句柄池示例 (`genSlab.cp`)

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

### 零堆分配服务/子进程监督退避与健康探针状态机示例 (`supervisor.cp`)

面向守护进程、Sidecar 与 Worker 生命周期编排的 100% 纯 CP 零动态堆分配监督状态机（支持 32 槽位、7 态生命周期、指数退避+确定性抖动、健康探针阈值裁决、滑动窗口隔离态与时间片动作调度）：

- **导出符号**：`PROC_SUP_MAX_SLOTS` (`32`)、`PROC_SUP_WINDOW_HISTORY` (`16`)、`PROC_SUP_INVALID_SLOT` (`0xFFFF`)、`ProcSupState`（`stopped | starting | healthy | degraded | backoff_waiting | quarantined | stopping`）、`ProcSupActionKind`（`none | spawn | send_graceful_stop | force_stop_after_grace`）、`ProcSupRestartPolicy`（`always | on_failure | never`）、`ProcSupStatus`、`ProcSupConfig`（`isValid`）、`ProcSupSlot`、`ProcSupAction`、`ProcSupStats`、`ProcSupervisor`（`init`, `registerService`, `startService`, `requestGracefulStop`, `recordProbeResult`, `recordProcessExit`, `resetQuarantine`, `pollNextAction`, `getSlot`, `stats`）。

```cp
import {
    ProcSupConfig,
    ProcSupActionKind,
    ProcSupervisor,
} from "cap:sys";

pub fn demoProcSupervisor() {
    let sup = ProcSupervisor.init();
    const slot = sup.registerService(ProcSupConfig{
        service_id: 101,
        initial_backoff_ms: 1000,
        max_backoff_ms: 30000,
        probe_success_threshold: 2,
        probe_failure_threshold: 3,
    }) ?? return;

    * = sup.startService(slot, 1000);
    const act = sup.pollNextAction(1000);
    if (act.kind == ProcSupActionKind.spawn) {
        // 拉起子进程后上报健康探针
        * = sup.recordProbeResult(slot, true, 1500);
        * = sup.recordProbeResult(slot, true, 2000); // 转 healthy
    }
}
```

### 零分配黑匣子二进制事件环形飞行记录器 (`flightLog.cp`)

面向系统崩溃取证与故障现场回放的 100% 纯 CP 零动态堆分配紧凑二进制环形飞行记录器：

- **常量与枚举**：
  - `FLIGHT_LOG_RING_BYTES = 8192`（8 KiB 2 的幂位掩码环形缓冲）、`FLIGHT_LOG_MAX_PAYLOAD = 256`、`FLIGHT_LOG_RECORD_HEADER_SIZE = 22`、`FLIGHT_LOG_DUMP_MAGIC = 0x314C4643` (`"CFL1"`)、`FLIGHT_LOG_DUMP_HEADER_SIZE = 32`、`FLIGHT_LOG_DUMP_TRAILER_SIZE = 4`
  - `FlightSeverity`（`trace | debug | info | warn | err | fatal`）
  - `FlightRecordStatus`（`ok | frozen | payload_too_large`）
  - `FlightDumpStatus`（`ok | invalid_magic | invalid_version | truncated | checksum_mismatch | corrupt_record`）
- **数据模型与离线校验函数**：
  - `FlightEvent`（`seq_id`, `timestamp_ns`, `event_kind`, `severity`, `payload_len`, `payload`, `payloadSlice()`）
  - `FlightDumpMeta`、`FlightLogStats`
  - `verifyBinaryDump(dump): FlightDumpMeta` / `parseBinaryDump(dump, out_events): usize?`：校验 Magic `"CFL1"`、版本、尾部 IEEE CRC32 与全部嵌入式记录帧结构及严格递增 `seq_id`。
- **核心引擎 `FlightRecorder`**：
  - `init()` / `recordEvent(*_, timestamp_ns, event_kind, severity, payload)`：单调递增 `seq_id`，缓冲满时自动驱逐最老记录并精确累计 `dropped_count`。
  - `snapshotEvents(after_seq_id, max_recent, out_events)`：支持按 `after_seq_id` 增量导出或取最近 N 条有序回放。
  - `freeze()` / `unfreeze()` / `isFrozen()` / `clear()`：崩溃/严重故障时冻结黑匣子防止后续日志冲刷现场。
  - `exportBinaryDump(out_buf)` / `stats()`：导出带尾部 CRC32 的自包含二进制转储。

```cp
import {
    FlightSeverity,
    FlightDumpStatus,
    FlightRecorder,
    verifyBinaryDump,
} from "cap:sys";

pub fn demoFlightRecorder() {
    let rec = FlightRecorder.init();
    * = rec.recordEvent(1_000, 10, FlightSeverity.info as u8, "worker:started");
    * = rec.recordEvent(2_000, 99, FlightSeverity.fatal as u8, "panic:oom");
    rec.freeze(); // 冻结黑匣子保护现场

    let dump: [512]u8 = [0; 512];
    let dump_s: []u8 = dump[0..];
    if (n := rec.exportBinaryDump(*dump_s)) {
        const meta = verifyBinaryDump(dump[0..n]);
        * = (meta.status == FlightDumpStatus.ok && meta.frozen); // true
    }
}
```

### 零分配多优先级防饥饿老化任务队列与背压调度器 (`workQueue.cp`)

面向高负载任务调度与后台作业编排的 100% 纯 CP 零动态堆分配多优先级工作队列：

- **常量与枚举**：
  - `WORK_QUEUE_MAX_CAPACITY = 128`、`WORK_QUEUE_MAX_PRIORITY = 3`（优先级 `0..3`）
  - `WorkBackpressurePolicy`（`reject_new | drop_lowest_priority | drop_oldest`）
  - `WorkEnqueueStatus`（`accepted | accepted_after_evict | rejected_full | rejected_invalid_id | rejected_duplicate_id | rejected_invalid_priority | rejected_already_expired`）
- **数据模型**：
  - `WorkQueueConfig`（`capacity`, `aging_step_ms`, `backpressure`；提供 `defaults()`, `isValid()`）
  - `WorkTask`（`task_id`, `priority`, `enqueued_ms`, `deadline_ms`, `payload_id`, `cancelled`, `occupied`, `seq`）
  - `WorkEnqueueOutcome`（`status`, `evicted_task_id`, `isAccepted()`）、`WorkDequeueItem`（`task`, `effective_priority`, `wait_ms`）、`WorkQueueStats`（占用与累计遥测指标）
- **核心引擎 `WorkQueue`**：
  - `init(config): WorkQueue?`：校验 `1 <= capacity <= 128`，非法返回 `null`。
  - `enqueue(*_, task_id, priority, now_ms, deadline_ms, payload_id): WorkEnqueueOutcome`：拦截 `task_id == 0`、非法优先级、重复活跃 ID 与已过期任务；满队列时优先回收已取消/过期槽位，仍满则按 `WorkBackpressurePolicy` 执行背压。
  - `dequeueNext(*_, now_ms): WorkDequeueItem?` / `peekNext(now_ms): WorkDequeueItem?`：自动跳过已取消与超过 `deadline_ms` 的任务；等待时间每超过 `aging_step_ms` 自动 +1 提升有效优先级（封顶 3），彻底消除饥饿。
  - `cancelTask(*_, task_id): bool` / `sweepExpired(*_, now_ms): usize` / `findTask(task_id)` / `len()` / `isEmpty()` / `isFull()` / `capacity()` / `clear(*_)` / `stats()`：支持精确取消、批量过期清扫与内置单调时钟防回退保护。

```cp
import {
    WorkBackpressurePolicy,
    WorkQueueConfig,
    WorkQueue,
} from "cap:sys";

pub fn demoWorkQueue() {
    const cfg = WorkQueueConfig{
        capacity: 32,
        aging_step_ms: 500,
        backpressure: WorkBackpressurePolicy.drop_lowest_priority,
    };
    let q = WorkQueue.init(cfg) ?? return;
    * = q.enqueue(1001, 0, 1000, 0, 10); // 低优任务
    * = q.enqueue(1002, 2, 2000, 0, 20); // 高优任务
    // 经过 1500ms 后（3 个 aging_step_ms），任务 1001 有效优先级升至 3，优先出队防饥饿
    if (item := q.dequeueNext(2500)) {
        * = item.task.task_id; // 1001
    }
}
```

### 零分配 POSIX / Node `fs.statfs` 文件系统容量与 Inode 模型 (`statFs.cp`)

吸收自 `/data/dev/bun/src/runtime/node/StatFS.cp`（参考 `/data/dev/bun/docs/runtime/nodejs-compat.mdx` 中 `node:fs` 兼容说明）：

- **常量与错误集**：`FS_MAGIC_EXT4` / `FS_MAGIC_TMPFS` / `FS_MAGIC_BTRFS` / `FS_MAGIC_XFS` / `FS_MAGIC_OVERLAYFS` / `FS_MAGIC_NFS` / `FS_MAGIC_PROC` / `FS_MAGIC_SYSFS` / `FS_MAGIC_ZFS`、`StatFsError`（`InvalidBlockSize`/`InconsistentCounts`）；
- **核心类型与函数**：`statFsTypeName(fstype)`、`StatFsSmall`（32 位紧凑快照）、`StatFsBig`（64 位无截断快照：`validate`/`toSmall`/`fsTypeName`/`totalBytes`/`freeBytes`/`availableBytes`/`usedBytes`/`usedPermille`/`availablePermille`/`inodeUsedPermille`/`isLowSpace`）；
- **运维最佳实践**：在大容量磁盘（> 8 TiB，4 KiB 块）场景下始终优先使用 `StatFsBig`（对应 `fs.statfs(..., { bigint: true })`）避免 32 位块数截断；磁盘告警应基于 `bavail`（非特权用户可用块）而非 `bfree`（含 root 保留块），使用 `isLowSpace(100)` 检测可用空间低于 10%。

```cp
import { FS_MAGIC_EXT4, StatFsBig } from "cap:sys";

const fs_info = StatFsBig{
    fstype: FS_MAGIC_EXT4,
    bsize: 4096,
    blocks: 1_000_000,
    bfree: 200_000,
    bavail: 150_000,
    files: 500_000,
    ffree: 400_000,
};
* = fs_info.validate()?;
const low = fs_info.isLowSpace(100); // false (150 permille >= 100)
* = low;
```

### 零分配 libuv / Node.js 系统与 DNS 错误码映射 (`libuvErrorMap.cp`)

吸收自 `/data/dev/bun/src/sys/libuv_error_map.cp`（同步自 `uv.h` 与 `/data/dev/bun/docs/runtime/archive.mdx`、`file-io.mdx` 中 Node.js/Bun 系统错误码最佳实践）：

- **常量与类型**：`LIBUV_ERROR_COUNT`（84 条权威映射）、`LIBUV_UNKNOWN_MESSAGE`（`"unknown error"`）、`LibuvErrorCategory`（`Filesystem`/`Network`/`Dns`/`Permission`/`Resource`/`Io`/`Argument`/`Process`/`Unknown`）、`LibuvErrno`（完整 84 项 POSIX 与 `EAI_*` DNS 错误码枚举）、`LibuvErrorEntry`、`LibuvErrorMap`；
- **核心 API**：
  - `LibuvErrorMap.entryFor(code)` / `entryAt(index)` / `entryCount()`
  - `LibuvErrorMap.codeName(code)` / `messageFor(code)` / `categoryFor(code)` / `isDnsError(code)` / `isRetryable(code)`
  - `LibuvErrorMap.lookupByName(name)` / `lookupByNameIgnoreCase(name)` / `fromCodeName(name)` / `messageByName(name)`
  - `LibuvErrorMap.formatError(code, syscall, path, out)`：零堆分配拼装 Node.js / Bun 标准错误诊断（如 `"ENOENT: no such file or directory, open '/etc/missing'"`），目标缓冲区不足时安全截断不越界；
- **Bun / Node.js 实战最佳实践**：
  - 按 `error.code`（如 `"EACCES"`、`"ENOENT"`、`"EEXIST"`）而非平台相关的裸数值 errno 进行分支判断，确保 Linux / macOS / Windows 跨平台行为一致；
  - 对瞬态系统/网络错误（`LibuvErrorMap.isRetryable(code)`，涵盖 `EAGAIN`、`EINTR`、`EBUSY`、`EAI_AGAIN`、`ECONNRESET`、`ETIMEDOUT` 等）实施指数退避重试，对权限/路径类永久错误（`EACCES`、`ENOENT`、`EISDIR`）快速失败并透出规范化诊断信息。

```cp
import { LibuvErrno, LibuvErrorMap } from "cap:sys";

let buf: [128]u8 = [0; 128];
const msg = LibuvErrorMap.formatError(.ENOENT, "open", "/tmp/missing.txt", buf[0..]);
// => "ENOENT: no such file or directory, open '/tmp/missing.txt'"
const retryable = LibuvErrno.EAI_AGAIN.isRetryable(); // true
* = msg;
* = retryable;
```

### 零分配 Node.js `fs.constants`、CopyfileFlags 与 POSIX FileMode (`fsConstants.cp`)

吸收自 `/data/dev/bun/src/runtime/node/node_fs_constant.cp`（参考 `/data/dev/bun/docs/runtime/file-io.mdx` 与 `nodejs-compat.mdx` 中文件权限与 CoW 克隆拷贝最佳实践）：

- **常量集**：
  - 拷贝标志：`COPYFILE_EXCL`、`COPYFILE_FICLONE`、`COPYFILE_FICLONE_FORCE`、`COPYFILE_VALID_MASK`、`UV_FS_O_FILEMAP`
  - 文件类型掩码：`FS_IFMT`、`FS_IFREG`、`FS_IFDIR`、`FS_IFLNK`、`FS_IFIFO`、`FS_IFSOCK`、`FS_IFCHR`、`FS_IFBLK`
  - 权限位：`S_IRWXU`/`S_IRUSR`/`S_IWUSR`/`S_IXUSR`、`S_IRWXG`/`S_IRGRP`/`S_IWGRP`/`S_IXGRP`、`S_IRWXO`/`S_IROTH`/`S_IWOTH`/`S_IXOTH`、`S_ISUID`/`S_ISGID`/`S_ISVTX`、`FS_PERM_MASK`
- **核心类型**：
  - `CopyfileFlags`：`none`/`fromBits`/`isValid`/`shouldntOverwrite`/`wantsClone`/`isForceClone`/`withExclusive`/`withClone`/`withForceClone`
  - `FileMode`：`init`/`fromTypeAndPerms`/`fileTypeBits`/`permBits`/`rwxBits`/`isReg`/`isDir`/`isSymlink`/`isFifo`/`isSocket`/`isCharDevice`/`isBlockDevice`/`typeChar`/`canOwner*`/`canGroup*`/`canOther*`/`hasSetuid`/`hasSetgid`/`hasSticky`/`applyUmask`/`formatSymbolic(out)`（10 字符 `ls -l` 权限串，支持 `s/S/t/T` 特殊位）/`parseOctal(input)`（支持 `"755"` / `"0755"` / `"0o644"` / `"4755"`）；
- **实战最佳实践**：
  - 优先使用 `COPYFILE_FICLONE`（而非 `COPYFILE_FICLONE_FORCE`）以在 APFS/Btrfs/XFS 上自动启用零拷贝 reflink，同时在不支持 CoW 的文件系统（如 ext4/tmpfs）上平滑回退至常规拷贝；
  - 创建敏感凭据文件时配合 `FileMode.fromTypeAndPerms(FS_IFREG, 0o600)` 与 `COPYFILE_EXCL` 防止意外覆盖和过度开放权限。

```cp
import { FS_IFDIR, COPYFILE_EXCL, COPYFILE_FICLONE, CopyfileFlags, FileMode } from "cap:sys";

const flags = CopyfileFlags.none().withExclusive().withClone();
const mode = FileMode.fromTypeAndPerms(FS_IFDIR, 0o755);
let buf: [10]u8 = [0; 10];
const sym = mode.formatSymbolic(buf[0..]); // "drwxr-xr-x"
* = flags.shouldntOverwrite(); // true
* = sym;
```

---

## 零分配多租户隔板并发隔离器与 AIMD 自适应并发守卫 (`bulkhead.cp`)

面向多租户服务端与上游依赖故障隔离的 100% 纯 CP 零动态堆分配 Bulkhead 隔板隔离器与 AIMD（Additive Increase / Multiplicative Decrease）自适应并发守卫：

- **常量与枚举**：
  - `BULKHEAD_MAX_COMPARTMENTS = 32`、`BULKHEAD_MAX_QUEUE_PER_COMP = 32`
  - `BulkheadAcquireStatus`（`granted | queued | rejected_full | unknown_compartment | invalid_arg`）
- **配置与数据模型**：
  - `BulkheadConfig`（`max_concurrent`, `max_queue`, `wait_timeout_ms`, `adaptive`, `min_concurrent`, `hard_max_concurrent`, `target_latency_ms`, `backoff_permille`, `healthy_streak_for_inc`；提供 `fixed(...)`, `adaptiveDefaults(...)`, `isValid()`）
  - `BulkheadAcquireResult`（`status`, `queue_position`, `in_flight`, `current_limit`）
  - `BulkheadReleaseResult`（`released`, `promoted_req_id`, `expired_skipped`, `in_flight`, `current_limit`）
  - `BulkheadCompartmentSnapshot` / `BulkheadGlobalStats`
- **核心引擎 `BulkheadGuard`**：
  - `init()` / `reset(*_)` / `registerCompartment(*_, comp_id, cfg)` / `removeCompartment(*_, comp_id)`
  - `tryAcquire(*_, comp_id, req_id, now_ms)`: 申请许可；自动清扫已超时排队者，并发满时按 FIFO 入队，队满返回 `rejected_full`，单舱饱和绝不影响其他隔板。
  - `release(*_, comp_id, req_id, latency_ms, failed, now_ms)`: 归还许可，按需执行 AIMD 自适应调参（低延迟健康连续达标加性递增 `+1`，高延迟或失败乘性回退至 `min_concurrent`），并自动按 FIFO 跳过已超时者提拔下一等待者返回 `promoted_req_id`。
  - `reapExpiredWaiters(*_, comp_id, now_ms)` / `compartmentSnapshot(comp_id)` / `stats()`。

```cp
import {
    BulkheadConfig,
    BulkheadAcquireStatus,
    BulkheadGuard,
} from "cap:sys";

pub fn demoBulkheadGuard() {
    let guard = BulkheadGuard.init();
    * = guard.registerCompartment(1, BulkheadConfig.fixed(2, 4, 1000));
    * = guard.registerCompartment(2, BulkheadConfig.adaptiveDefaults(4, 2, 16, 150));

    const r1 = guard.tryAcquire(1, 101, 100); // granted
    const r2 = guard.tryAcquire(1, 102, 100); // granted
    const r3 = guard.tryAcquire(1, 103, 100); // queued (pos=1)
    if (r3.status == BulkheadAcquireStatus.queued) {
        const rel = guard.release(1, 101, 50, false, 150);
        * = rel.promoted_req_id; // 103 自动提拔!
    }
    * = r1; * = r2;
}
```

### 哈希加速目录遍历跳过过滤器与单缓冲路径栈 (`walkerSkip.cp`)

> 吸收自 `/data/dev/bun/src/sys/walker_skippable.cp`，同步参考 `/data/dev/bun/docs/runtime/file-io.mdx` 文件系统遍历最佳实践

- **导出符号**：`WALKER_MAX_SKIP_RULES` (64)、`WALKER_MAX_DEPTH` (64)、`WALKER_PATH_BUF_CAP` (4096)、`WalkerEntryKind`、`WalkerDecision`、`WalkerSkipError`、`walkerHashName`、`WalkerSkipFilter`、`WalkerPathStack`；
- **核心设计**：
  1. **零哈希快速短路**：`WalkerSkipFilter` 维护 `skip_dirnames`、`skip_filenames`、`skip_all` 三张定长 64 位种子哈希表；当对应规则表为空时直接跳过哈希计算；符号链接（`SymLink`）同时匹配目录与文件黑名单（`skip_all`）；
  2. **NFS / FUSE `DT_UNKNOWN` 降级**：开启 `setResolveUnknown(true)` 后，遇到 `WalkerEntryKind.Unknown` 返回 `WalkerDecision.ResolveUnknown`，提示上层回退 `lstatat` 解析真实类型；
  3. **单缓冲零堆分配路径栈**：`WalkerPathStack` 复用单个 4096 字节缓冲区，通过 `pushDir` / `joinEntry` / `popDir` 收缩复写 `dir_len`，杜绝深层目录扫描时的重复字符串分配；
  4. **文件 I/O 实战提示（同步自 Bun `file-io.mdx`）**：对大规模目录树扫描优先使用延迟加载与过滤前置（`withCommonIgnores` 跳过 `.git`、`node_modules`、`.cap`），避免对忽略目录触发不必要的 `open` / `stat` 系统调用。

```cp
import { WalkerSkipFilter, WalkerPathStack, WalkerEntryKind, WalkerDecision } from "cap:sys";

let filter = WalkerSkipFilter.withCommonIgnores(42);
const d1 = filter.evaluate(WalkerEntryKind.Directory, "node_modules"); // WalkerDecision.Skip
let stack = WalkerPathStack.initPosix();
stack.pushDir("src")?;
if (full := stack.joinEntry("main.cp")) {
    * = full; // "src/main.cp"
}
* = d1;
```

### 多租户赤字轮询 DRR 加权公平任务调度器 (`fairSched.cp`)

面向多租户并发请求/AI Token/带宽公平调度的 100% 纯 CP 零动态堆分配 Deficit Round Robin (DRR) 加权公平调度器：

- **常量与枚举**：
  - `FAIR_SCHED_MAX_TENANTS = 32`、`FAIR_SCHED_QUEUE_CAP = 32`、`FAIR_SCHED_BPS_SCALE = 10000`
  - `FairSchedOpStatus`（`ok | invalid_input | tenant_exists | tenant_not_found | tenant_table_full | queue_full`）
- **数据模型**：
  - `FairTask`（`tenant_id`, `task_id`, `cost`, `enqueued_ms`）
  - `FairTenantStats`（每租户 `quantum`, `deficit`, `queued_count`, `enqueued_total`, `dequeued_total`, `dropped_total`, `scheduled_cost_total`, `in_active_ring`）
  - `FairSchedStats`（全局统计与加权 Jain 公平性指数 `jain_fairness_bps`）
- **核心引擎 `DeficitRoundRobinScheduler`**：
  - `init()` / `reset(*_)` / `registerTenant(*_, tenant_id, quantum)` / `setQuantum(*_, tenant_id, new_quantum)` / `removeTenant(*_, tenant_id)`
  - `enqueue(*_, tenant_id, task_id, cost, now_ms)`：每租户 32 槽 FIFO 环形入队与队列满背压拒绝（`queue_full`）。
  - `dequeue(*_): FairTask?`：标准 O(1) 活跃环赤字轮询，回合累加 `deficit += quantum`，队首任务 `cost <= deficit` 即出队并扣减赤字，队列排空自动归零赤字并移出活跃环，严格按权重比例分配大/小任务吞吐且零饥饿。
  - `tenantStats(tenant_id)` / `jainFairnessBps()` / `stats()`

```cp
import {
    DeficitRoundRobinScheduler,
} from "cap:sys";

pub fn demoFairScheduler() {
    let sched = DeficitRoundRobinScheduler.init();
    * = sched.registerTenant(1, 200); // 权重 2x
    * = sched.registerTenant(2, 100); // 权重 1x

    * = sched.enqueue(1, 101, 100, 1000);
    * = sched.enqueue(1, 102, 100, 1001);
    * = sched.enqueue(2, 201, 100, 1002);

    // 租户 1 本轮 quantum=200 连续出队 101 & 102，随后轮转租户 2 出队 201
    * = sched.dequeue();
    * = sched.dequeue();
    * = sched.dequeue();
    * = sched.jainFairnessBps(); // 10000 (100% 加权公平)
}
```

### 零分配进程树、Subreaper 孤儿托管与自底向上优雅停机编排器 (`procTree.cp`)

面向容器初始化进程、Agent 沙箱与多级子进程监管的 100% 纯 CP 零动态堆分配进程树管理与停机编排引擎：

- **常量与枚举**：
  - `PROC_TREE_MAX_NODES = 64`、`PROC_TREE_NO_PID = 0`、`PROC_TREE_DEFAULT_INIT_PID = 1`
  - `ProcNodeState`（`running | stopping | zombie | reaped`；配套 `formatProcNodeState(state)`）
  - `ProcTreeOpStatus`（`ok | invalid_pid | duplicate_pid | parent_not_found | parent_not_alive | cycle_detected | capacity_full | not_found | invalid_state`）
- **数据模型**：
  - `ProcNode`（`pid`, `ppid`, `pgid`, `state`, `exit_code`, `is_subreaper`, `started_at_ms`, `exited_at_ms`; `isAlive()`）
  - `ReapedProcInfo`、`ProcExitReport`、`ProcTreeStats`
- **核心引擎 `ProcessTree`**：
  - `init(root_pid, now_ms): ProcessTree?` / `spawn(*_, pid, ppid, pgid, is_subreaper, now_ms)` / `reparent(*_, pid, new_ppid)`（含显式祖先链环路防护）/ `setSubreaper(*_, pid, enabled)` / `markStopping(*_, pid)` / `getNode(pid)`
  - `planShutdownOrder(subtree_root_pid, *out_pids): usize`：纯迭代零递归 BFS + 层次反转，按由深到浅“先子后父”确定性顺序输出优雅停机列表。
  - `collectProcessGroup(pgid, *out_pids): usize`：按升序 PID 批量筛选进程组内全部存活进程。
  - `markExited(*_, pid, exit_code, now_ms): ProcExitReport`：进入 `zombie` 状态并将全部直系子进程原子重挂到祖先链上最近的存活 `is_subreaper`（或根 init）节点，彻底杜绝孤儿逃逸。
  - `reapZombies(*_, parent_pid, *out_reaped): usize` / `stats(): ProcTreeStats`

```cp
import {
    ReapedProcInfo,
    ProcessTree,
} from "cap:sys";

pub fn demoProcessTree() {
    let tree = ProcessTree.init(1, 1000) ?? return;
    * = tree.spawn(10, 1, 10, true, 1010);  // subreaper
    * = tree.spawn(20, 10, 10, false, 1020);
    * = tree.spawn(30, 20, 10, false, 1030);

    // 1. 纯迭代自底向上优雅停机顺序: [30, 20, 10]
    let order: [8]u32 = [0; 8];
    let order_s: []u32 = order[0..];
    * = tree.planShutdownOrder(10, *order_s);

    // 2. 中间父进程 20 退出 -> 孤儿 30 自动重挂至最近的 subreaper (10)
    * = tree.markExited(20, 1, 2000);
    let reaped: [4]ReapedProcInfo = [ReapedProcInfo{}; 4];
    let reaped_s: []ReapedProcInfo = reaped[0..];
    * = tree.reapZombies(10, *reaped_s);
}
```

## 零分配 Linux cgroup v2 CPU/内存配额、PSI 压力解析与自适应并发预算引擎 (`cgroupLimit.cp`)

面向 Kubernetes / 容器化部署运行时的 100% 纯 CP 零动态堆分配资源配额与 PSI 压力感知引擎，纯整数定点解析零浮点误差并给出确定性并发度与背压裁决：

- **CPU 配额解析 (`cpu.max`)**：
  - `CgroupCpuQuota`（`is_unlimited`, `quota_us`, `period_us`, `effectiveMillicores()`, `recommendedWorkers(host_cores)`）
  - `parseCgroupCpuMax(content): CgroupCpuQuota?`（支持 `"max 100000"` 与 `"250000 100000"`，拒绝零周期/负值/畸形输入）
- **内存配额解析 (`memory.max` / `memory.high` / `memory.current`)**：
  - `CgroupMemoryLimit`（`is_unlimited`, `limit_bytes`）、`parseCgroupMemoryValue(content): CgroupMemoryLimit?`
- **PSI (Pressure Stall Information) 定点解析 (`/proc/pressure/memory|cpu|io`)**：
  - `CGROUP_BPS_SCALE = 10000`、`PsiMetrics`（`avg10_bps`, `avg60_bps`, `avg300_bps`, `total_us`）、`PsiSnapshot`（`has_some`, `some`, `has_full`, `full`）
  - `parsePsiPressure(content): PsiSnapshot?`（纯整数定点数解析 `12.34% -> 1234 bps`，零浮点误差）
- **容器自适应负载/背压裁决器**：
  - `CgroupPressureLevel`（`nominal | elevated | critical | oom_imminent`）
  - `CgroupBudgetAdvice`（`pressure_level`, `memory_util_bps`, `effective_workers`, `headroom_bytes`, `should_shed_load`, `should_pause_background`）
  - `evaluateCgroupBudget(cpu, mem_max, mem_current_bytes, psi, host_cores): CgroupBudgetAdvice`

```cp
import {
    parseCgroupCpuMax,
    parseCgroupMemoryValue,
    parsePsiPressure,
    evaluateCgroupBudget,
} from "cap:sys";

pub fn demoCgroupBudget() {
    const cpu = parseCgroupCpuMax("250000 100000\n") ?? return;
    const mem = parseCgroupMemoryValue("1073741824\n") ?? return; // 1 GiB
    const psi = parsePsiPressure(
        "some avg10=1.25 avg60=0.50 avg300=0.10 total=12345\nfull avg10=0.00 avg60=0.00 avg300=0.00 total=0\n",
    ) ?? return;

    const advice = evaluateCgroupBudget(cpu, mem, 536870912, psi, 16);
    * = advice.effective_workers; // 3 (2.5 vCPU -> ceil = 3)
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

