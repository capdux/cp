# CP 标准库：平台信息与内核特性探测 (`cap:platform`)

## 一、 架构总览与设计准则

`cap:platform` 是 CP 语言官方内置的跨平台操作系统探测、内核版本解析与系统能力检测标准库，吸收自 Bun `src/platform/`（`linux.zig`、`darwin.zig`）。该库向事件循环、文件 I/O 引擎（如 `io_uring` / `copy_file_range` / `statx` 探测）、进程管理工具链提供零堆分配的系统元信息与内核特性判定接口。

### 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/platform/os_info.cp` | `// Source: /data/dev/bun/src/platform/linux.cp` | `OsKind`、`ArchKind`、`Endianness`、`KernelVersion` 与 `PlatformInfo` 探测引擎 |
| `lib/platform/index.cp` | `// Source: /data/dev/bun/src/platform/linux.cp` | `cap:platform` 对外统一导出入口 |

---

## 二、 核心 API 与设计语义

### 1. 枚举分类 (`OsKind` / `ArchKind` / `Endianness`)

- **`OsKind`**：`Linux`、`MacOS`、`Windows`、`FreeBSD`、`Wasi`、`Unknown`，提供 `.name()` 与 `.isPosix()`。
- **`ArchKind`**：`X86_64`、`Aarch64`、`Riscv64`、`Wasm32`、`Unknown`，提供 `.name()`、`.pointerBits()`、`.is64Bit()`。
- **`Endianness`**：`Little`、`Big`，提供 `.isLittle()`、`.isBig()`。

### 2. 内核版本解析与比较 (`KernelVersion`)

- `KernelVersion.parse(release_str)`: 支持 `"6.8.12-generic"`、`"5.15.0"`、`"6.1"` 等释放字符串解析，忽略 `-generic` / `+deb` 等后缀。
- `compare(other)`：返回 `-1` / `0` / `1` 三态比较结果。
- `isAtLeast(major, minor)` / `isAtLeastPatch(major, minor, patch)`：判断当前内核是否满足最低版本门槛。
- `format(buf)`：将版本号格式化为 `"major.minor.patch"`。

### 3. 运行时与编译期探测 (`PlatformInfo`)

- `currentOs()` / `currentArch()` / `endianness()`：编译期零开销判定当前目标平台、架构与字节序。
- `pageSize()` / `cpuCount()`：查询宿主机内存页大小与逻辑 CPU 核心数。
- `kernelVersion()`：在 Linux 上直调 `std.os.linux.uname` 解析真实内核版本。
- **Linux 内核特性探测**：
  - `supportsIoUring()`（>= 5.1）
  - `supportsCopyFileRange()`（>= 4.5）
  - `supportsStatx()`（>= 4.11）
  - `supportsPidfd()`（>= 5.3）
- `executablePath(buf)`：Linux 下直接通过 `std.os.linux.readlink("/proc/self/exe", ...)` 读取当前进程可执行文件真实路径。
- `hostname(buf)`：通过 `uname` `nodename` 获取宿主机主机名。

---

## 三、 可运行代码示例

```typescript
import {
    OsKind,
    ArchKind,
    Endianness,
    KernelVersion,
    PlatformInfo,
} from "cap:platform";

pub fn demoPlatform(): ! {
    // 1. 解析并比较内核版本
    const kv = KernelVersion.parse("6.8.12-generic")?;
    const is_modern = kv.isAtLeast(5, 1); // true
    * = is_modern;

    // 2. 查询宿主机信息
    const os_kind = PlatformInfo.currentOs();
    const arch_kind = PlatformInfo.currentArch();
    const page_sz = PlatformInfo.pageSize();
    const cpus = PlatformInfo.cpuCount();
    const endian = PlatformInfo.endianness();
    * = os_kind;
    * = arch_kind;
    * = page_sz;
    * = cpus;
    * = endian;

    // 3. Linux 内核能力、可执行文件路径与主机名
    if (os_kind == OsKind.Linux) {
        const live_kv = PlatformInfo.kernelVersion()?;
        * = live_kv;
        * = PlatformInfo.supportsIoUring();
        * = PlatformInfo.supportsStatx();

        let exe_buf: [1024]u8 = undefined;
        const exe = PlatformInfo.executablePath(&exe_buf)?;
        * = exe;

        let host_buf: [128]u8 = undefined;
        const host = PlatformInfo.hostname(&host_buf)?;
        * = host;
    }
}
```
