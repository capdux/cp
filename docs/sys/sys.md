# CP 标准库：跨平台系统调用、文件句柄、元数据与进程控制手册 (`cap:sys`)

## 一、 架构总览与设计准则

`cap:sys` 是 CP 语言官方内置的底层系统调用与操作系统原语标准库，完全基于 Zig 0.17 原生 `std.c.*` 与 `std.os.linux.*` 构建（零废弃旧 API 依赖），提供强类型文件描述符 `FD`、RAII 文件句柄 `File`、POSIX 元数据自省 `Stat`（`stat` / `lstat` / `fstat`）、完整文件与目录系统调用原语（`open`/`close`/`read`/`write`/`pread`/`pwrite`/`lseek`/`ftruncate`/`mkdir`/`rmdir`/`unlink`/`rename`/`symlink`/`readlink`/`chmod`/`access`/`dup`/`dup2`/`getcwd`/`chdir`）、内核零拷贝复制（`copyFile` / `copyFileByFd` / `copyFileRange`）、进程与硬件信息原语（`getpid`/`getppid`/`getuid`/`getgid`/`uname`/`pageSize`/`cpuCount`/`monotonicNs`/`realtimeMs`）、防冲突临时文件工具集以及 POSIX 信号（`Signal`）。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP 系统编程与运行时底座 (`cap:sys`)                 │
├──────────────────────┬─────────────────────────┬────────────────────────┤
│ 文件与目录系统调用    │  进程、时钟与硬件原语    │  零拷贝、信号与临时资源 │
│ FD / File / Stat     │  getpid / getppid       │  copyFile / copyFileByFd│
│ open / close / read  │  getuid / getgid        │  copyFileRange         │
│ write / pread/pwrite │  uname() -> UnameInfo   │  getTempDir / makeTemp │
│ lseek / ftruncate    │  pageSize / cpuCount    │  Signal (1..31 枚举)   │
│ stat / lstat / fstat │  monotonicNs / realtime │  Sys.setAffinity(core) │
│ mkdir/rmdir/unlink   │                         │  Sys.Lib (dlopen/dlsym)│
│ rename/symlink/chmod │                         │                        │
│ readlink/access/dup2 │                         │                        │
│ getcwd / chdir       │                         │                        │
├──────────────────────┴─────────────────────────┴────────────────────────┤
│              Zig 0.17 std.c.* / std.os.linux.* 原生适配层               │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/sys/fd.cp` | `// Source: /data/dev/bun/src/sys/fd.cp` | `FD` 强类型描述符、`O_*` / `F_OK` / `R_OK` / `W_OK` / `X_OK` / `SEEK_*` 常量，以及 `read`/`write`/`writeAll`/`pread`/`pwrite`/`lseek`/`seekTo`/`seekBy`/`seekFromEnd`/`ftruncate`/`dup`/`dup2`/`sync`/`close` |
| `lib/sys/stat.cp` | `// Source: /data/dev/bun/src/sys/PosixStat.cp` | `Stat` 元数据结构与 `stat`/`lstat`/`fstat`（基于 `std.c.statx` 实现，不跟随符号链接的 `lstat`） |
| `lib/sys/file.cp` | `// Source: /data/dev/bun/src/sys/File.cp` | `File` 高阶句柄，`open`/`create`/`openReadOnly`/`close`/`ftruncate`/`lseek`/`sync`/`dup`/`stat`/`size`/`readToEndAlloc` |
| `lib/sys/fs_ops.cp` | `// Source: /data/dev/bun/src/sys/sys.cp` | 独立导出系统调用函数：`open`/`close`/`read`/`write`/`pread`/`pwrite`/`lseek`/`ftruncate`/`dup`/`dup2`/`mkdir`/`rmdir`/`unlink`/`rename`/`symlink`/`readlink`/`chmod`/`access`/`getcwd`/`chdir` |
| `lib/sys/process.cp` | `// Source: /data/dev/bun/src/platform/linux.cp` | 进程与硬件原语：`getpid`/`getppid`/`getuid`/`getgid`/`UnameInfo`/`uname`/`pageSize`/`cpuCount`/`monotonicNs`/`realtimeMs` |
| `lib/sys/copy_file.cp` | `// Source: /data/dev/bun/src/sys/copy_file.cp` | `copyFile` / `copyFileByFd` / `copyFileRange` |
| `lib/sys/tmp.cp` | `// Source: /data/dev/bun/src/sys/tmp.cp` | `getTempDir` / `makeTempFile` / `makeTempDir` / `removeFile` / `removeDir` |
| `lib/sys/signal.cp` | `// Source: /data/dev/bun/src/sys/SignalCode.cp` | 30 个标准 POSIX `Signal` 枚举、`description`、`toExitCode`、`fromNumber`、`fromName` |
| `lib/sys/index.cp` | `// Source: /data/dev/bun/src/sys/sys.zig & /data/dev/bun/src/sys/Error.cp` | `cap:sys` 统一导出、`Sys`/`sys` 命名空间、`Sys.setAffinity` 与 `Sys.Lib` |

---

## 二、 核心 API 矩阵

### 1. 文件与文件系统原语（`cap:sys` & `Sys.*`）

- **打开与关闭**：`open(path, flags, mode): !FD`、`close(fd)`、`Sys.dup(fd)` / `dupFd(fd)` / `fd.dup(): !FD`、`dup2(old_fd, new_fd): !FD`
- **读写与定位**：`read(fd, buf): !usize`、`write(fd, data): !usize`、`pread(fd, buf, offset): !usize`、`pwrite(fd, data, offset): !usize`、`lseek(fd, offset, whence): !u64`、`ftruncate(fd, length): !void`
- **元数据与链接**：`stat(path): !Stat`、`lstat(path): !Stat`、`fstat(fd): !Stat`、`symlink(target, link_path): !void`、`readlink(path, buf): ![]const u8`
- **目录与权限**：`mkdir(path, mode): !void`、`rmdir(path): !void`、`unlink(path): !void`、`rename(old_path, new_path): !void`、`chmod(path, mode): !void`、`access(path, mode): !void`、`getcwd(buf): ![]const u8`、`chdir(path): !void`
- **零拷贝复制**：`copyFileRange(src_fd, src_off, dest_fd, dest_off, len): !usize`、`copyFileByFd(src, dst)`、`copyFile(src_path, dst_path)`

### 2. 进程、系统信息与高精度时钟原语

- `getpid(): i32` / `getppid(): i32` / `getuid(): u32` / `getgid(): u32`
- `uname(): !UnameInfo`（提供 `sysnameStr()`, `nodenameStr()`, `releaseStr()`, `versionStr()`, `machineStr()`）
- `pageSize(): usize` / `cpuCount(): usize`
- `monotonicNs(): u64`（单调纳秒时钟，适用于耗时测量与超时调度）
- `realtimeMs(): i64`（Unix 纪元毫秒时间戳）

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    FD, O_RDWR, O_CREAT, O_TRUNC, O_CLOEXEC,
    F_OK, R_OK, W_OK, SEEK_SET,
    open, close, write, pread, lseek, ftruncate,
    stat, lstat, mkdir, rmdir, unlink, rename,
    symlink, readlink, chmod, access, Sys, dupFd, dup2,
    getcwd, chdir, copyFileRange,
    getpid, getppid, getuid, getgid, uname,
    pageSize, cpuCount, monotonicNs, realtimeMs,
    makeTempDir,
} from "cap:sys";

pub fn demoSysPrimitives()! {
    // 1. 进程与系统自省
    const pid = getpid();
    const ppid = getppid();
    const pagesz = pageSize();
    const cpus = cpuCount();
    const t0 = monotonicNs();
    const now_ms = realtimeMs();
    let info = try uname();
    * = pid; * = ppid; * = pagesz; * = cpus; * = t0; * = now_ms;
    * = info.sysnameStr();

    // 2. 临时目录、open/write/ftruncate/lseek/pread/dup/symlink/lstat/readlink/rename/chmod/access
    let dir_buf: [512]u8 = undefined;
    const tmp_dir = try makeTempDir("sys_demo", &dir_buf);
    defer rmdir(tmp_dir) catch {};

    let pbuf: [512]u8 = undefined;
    const fpath = try std.fmt.bufPrint(&pbuf, "{s}/a.txt", .{tmp_dir});
    const fd = try open(fpath, O_CREAT | O_RDWR | O_TRUNC | O_CLOEXEC, 0o644);
    * = try write(fd, "hello world");
    try ftruncate(fd, 5); // 截断为 "hello"
    * = try lseek(fd, 0, SEEK_SET);
    close(fd);

    try access(fpath, F_OK | R_OK | W_OK);
    try chmod(fpath, 0o600);

    let lbuf: [512]u8 = undefined;
    const link_path = try std.fmt.bufPrint(&lbuf, "{s}/a.link", .{tmp_dir});
    try symlink(fpath, link_path);
    const lst = try lstat(link_path);
    // lst.isSymLink() == true
    * = lst;
    try unlink(link_path);
    try unlink(fpath);
}
```
