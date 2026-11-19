# CP 标准库：跨平台系统调用、文件句柄、元数据与进程信号手册 (`cap:sys`)

## 一、 架构总览与设计准则

`cap:sys` 是 CP 语言官方内置的底层系统调用与操作系统原语标准库，提供强类型文件描述符 `FD`、RAII 文件句柄 `File`、POSIX 元数据自省 `Stat`（`stat` / `fstat`）、优先采用 Linux `copy_file_range` 内核零拷贝的高速文件复制引擎（`copyFile` / `copyFileByFd`）、防冲突临时文件与临时目录工具集（`getTempDir` / `makeTempFile` / `makeTempDir` / `removeFile` / `removeDir`）、完整 POSIX 信号枚举与名称解析（`Signal` / `fromNumber` / `fromName`），以及线程 CPU 亲和性绑定与动态库加载（`Sys.setAffinity` / `Sys.Lib`）。核心架构吸收自 Bun 系统抽象层（`/data/dev/bun/src/sys/`）。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP 系统编程与运行时底座 (`cap:sys`)                 │
├──────────────────────┬─────────────────────────┬────────────────────────┤
│ 文件与描述符子系统    │  零拷贝与临时资源管理    │  进程信号与底层控制     │
│ FD (stdin/out/err)   │  copyFile / copyFileByFd│  Signal (1..31 枚举)   │
│   read/write/writeAll│    Linux copy_file_range│    description/toExit  │
│   pread/pwrite/lseek │    16KB 分块回退引擎     │    fromNumber/fromName │
│ File (open/create)   │  getTempDir             │  Sys.setAffinity(core) │
│ Stat (stat / fstat)  │  makeTempFile/Dir       │  Sys.Lib (dlopen/dlsym)│
│   isFile/isDir/...   │  removeFile/removeDir   │                        │
├──────────────────────┴─────────────────────────┴────────────────────────┤
│                POSIX C-ABI / Linux Syscall 适配层 (std.c)               │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/sys/` 下全部 7 个源文件第 1 行均严格标记上游 Bun 源码谱系：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/sys/fd.cp` | `// Source: /data/dev/bun/src/sys/fd.cp` | `FD` 强类型描述符包装、`O_*` 打开标志常量及 `read`/`write`/`writeAll`/`pread`/`pwrite`/`seekTo`/`seekFromEnd`/`sync`/`close` |
| `lib/sys/stat.cp` | `// Source: /data/dev/bun/src/sys/PosixStat.cp` | `Stat` 元数据结构（`size`/`mode`/`mtime`/`atime`/`ctime`/`ino`/`dev`/`nlink`/`uid`/`gid`）、类型判定及 `stat`/`fstat` |
| `lib/sys/file.cp` | `// Source: /data/dev/bun/src/sys/File.cp` | `File` 高阶文件句柄，封装 `open`/`create`/`openReadOnly`/`close`、位置 I/O、`stat`/`size` 与 `readToEndAlloc` |
| `lib/sys/copy_file.cp` | `// Source: /data/dev/bun/src/sys/copy_file.cp` | `copyFile` / `copyFileByFd`：Linux 下优先 `copy_file_range` 内核零拷贝，不可用时无缝回退至 16KB 分块拷贝 |
| `lib/sys/tmp.cp` | `// Source: /data/dev/bun/src/sys/tmp.cp` | `getTempDir`（按 `TMPDIR` -> `TMP` -> `TEMP` -> `"/tmp"` 解析）、`makeTempFile`（`O_CREAT|O_EXCL` 安全防冲突）、`makeTempDir`、`removeFile`、`removeDir` |
| `lib/sys/signal.cp` | `// Source: /data/dev/bun/src/sys/SignalCode.cp` | 30 个标准 POSIX `Signal` 枚举、`description()`、`toExitCode()`（`128 + signo`）、`label()` 及 `fromNumber`/`fromName` 双向解析 |
| `lib/sys/index.cp` | `// Source: /data/dev/bun/src/sys/sys.zig & /data/dev/bun/src/sys/Error.cp` | `cap:sys` 统一导出、`Sys`/`sys` 命名空间、`Sys.setAffinity` 与 `Sys.Lib` 动态库加载器 |

---

## 二、 核心 API 与安全设计

### 1. 描述符与文件操作（`FD` & `File`）

- **标准流保护**：`FD.close()` 仅关闭有效且非标准流（`fd > 2`）的描述符，杜绝误关 `stdin`/`stdout`/`stderr`。
- **完整写入保障**：`FD.writeAll(data)` 循环处理部分写入（Partial Write），直至全部字节落盘或遭遇 `error.WriteZero` / `error.WriteFailed`。
- **位置 I/O（无游标竞争）**：`pread(buf, offset)` 与 `pwrite(data, offset)` 不修改文件偏移量，天然适合并发或随机块读写。
- **RAII 清理**：推荐配合 `defer file.close();` 保障任何异常路径下句柄零泄漏。

### 2. 文件元数据自省（`Stat`）

`stat(path)` 与 `fstat(fd)` 返回统一跨平台 `Stat` 结构，通过 POSIX 文件类型掩码 `0o170000` 提供直观方法：
- `isFile()`（普通文件 `0o100000`）
- `isDir()`（目录 `0o040000`）
- `isSymLink()`（符号链接 `0o120000`）
- `isSocket()`（Unix 套接字 `0o140000`）
- `isFIFO()`（命名管道 `0o010000`）

### 3. 零拷贝文件复制（`copyFile` / `copyFileByFd`）

- 在 Linux 平台优先调用 `std.c.copy_file_range` 在内核页缓存内直接完成数据迁移（单次上限 1GiB，循环推进），完全绕过用户态缓冲区拷贝；
- 若跨文件系统或底层不支持 `copy_file_range`，自动透明回退至 16KB 栈缓冲 `read` + `writeAll` 循环，保证 100% 兼容。

### 4. 临时文件与 POSIX 信号（`tmp.cp` & `signal.cp`）

- **`makeTempFile(prefix, suffix, path_buf)`**：基于实时时间戳 + 单调递增序列号生成唯一路径，并使用 `O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC` + `0o600` 权限原子创建，最多重试 100 次，消除 TOCTOU 竞争条件。
- **`Signal`**：覆盖 `SIGHUP (1)` 至 `SIGSYS (31)`（跳过未分配的 16）；`fromName` 同时兼容带 `SIG` 前缀（`"SIGTERM"`）与短名（`"TERM"`）；`toExitCode()` 遵循 POSIX 惯例返回 `128 + signal_num`（如 `SIGINT -> 130`，`SIGTERM -> 143`）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    File,
    stat,
    copyFile,
    Signal,
    fromNumber,
    fromName,
    getTempDir,
    makeTempFile,
    makeTempDir,
    removeFile,
    removeDir,
} from "cap:sys";

pub fn demoSysSubsystem(): !void {
    // 1. 创建唯一临时文件、写入并随机位置读写
    let src_buf: [1024]u8 = undefined;
    let file = try makeTempFile("cap_demo", ".dat", &src_buf);
    const src_len = std.mem.indexOfScalar(u8, &src_buf, 0) orelse src_buf.len;
    const src_path = src_buf[0..src_len];

    try file.writeAll("0123456789ABCDEF");
    * = try file.pwrite("WXYZ", 4); // 就地覆盖偏移 4..8
    const st_fd = try file.stat();  // st_fd.size == 16, st_fd.isFile() == true
    * = st_fd;
    file.close();

    // 2. 创建临时目录并执行零拷贝文件复制
    let dir_buf: [1024]u8 = undefined;
    const tmp_dir = try makeTempDir("cap_dir", &dir_buf);
    let dst_buf: [1024]u8 = undefined;
    const dst_path = try std.fmt.bufPrint(&dst_buf, "{s}/copied.dat", .{tmp_dir});

    try copyFile(src_path, dst_path);
    const dst_st = try stat(dst_path);
    * = dst_st.size; // 16

    // 3. 清理临时文件与目录
    try removeFile(dst_path);
    try removeDir(tmp_dir);
    try removeFile(src_path);

    // 4. POSIX 信号解析
    const sig = fromName("TERM").?;    // Signal.SIGTERM
    const exit_code = sig.toExitCode(); // 143
    const by_num = fromNumber(2).?;    // Signal.SIGINT
    * = exit_code;
    * = by_num;
}
```

---

## 四、 与 Bun / Node.js API 对照表

| 功能特性 | Bun (`src/sys/` / `Bun.file`) | Node.js (`node:fs` / `node:os`) | CP 标准库 (`cap:sys`) |
| :--- | :--- | :--- | :--- |
| **强类型文件描述符** | `bun.FileDescriptor` (`src/sys/fd.cp`) | `number` fd / `fs.FileHandle` | `FD` (`read`/`writeAll`/`pread`/`pwrite`/`sync`) |
| **文件句柄与位置 I/O** | `sys.File` (`src/sys/File.cp`) | `fs.openSync` / `fs.readSync` | `File.open` / `create` / `openReadOnly` / `pread` / `pwrite` |
| **文件元数据自省** | `sys.stat` / `sys.fstat` (`PosixStat.cp`) | `fs.statSync` / `fs.fstatSync` | `stat(path)` / `fstat(fd)` -> `Stat` (`isFile`/`isDir`) |
| **零拷贝文件复制** | `sys.copyFile` (`copy_file_range`) | `fs.copyFileSync` | `copyFile(src, dst)` / `copyFileByFd(src_fd, dst_fd)` |
| **临时目录与文件** | `os.tmpdir()` / `src/sys/tmp.cp` | `os.tmpdir()` / `fs.mkdtempSync` | `getTempDir()` / `makeTempFile` / `makeTempDir` / `removeFile` / `removeDir` |
| **POSIX 信号映射** | `SignalCode` (`src/sys/SignalCode.cp`) | `os.constants.signals` | `Signal` / `fromNumber` / `fromName` / `toExitCode` |
| **动态库与 CPU 亲和性** | `dlopen` / `sched_setaffinity` | `dlopen` (N-API) | `Sys.Lib.open` / `Sys.setAffinity` |
