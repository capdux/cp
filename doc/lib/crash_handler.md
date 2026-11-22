# cap:crash_handler

## 1. 概述

`cap:crash_handler`（以及动态命名空间 `cap.crash_handler`）提供纯 CP 实现的工业级结构化崩溃诊断报告、栈帧符号格式化、零堆固定容量写入器与环形面包屑缓冲区。设计目标是在信号处理与异常现场实现零 `std.posix` 耦合、无堆分配截断保护，并支持人读文本与 RFC 8259 转义 JSON 诊断载荷导出。

## 2. 核心类型

| 类型 | 说明 |
| --- | --- |
| `SignalKind` | 崩溃信号枚举：`Segv`, `Abrt`, `Bus`, `Ill`, `Fpe`, `Trap`, `Panic`, `Unknown` |
| `CrashReason` | 崩溃原因联合体：`Panic`, `Unreachable`, `SegmentationFault`, `IllegalInstruction`, `BusError`, `FloatingPointError`, `Trap`, `Abort`, `OutOfMemory`, `AssertionFailed`, `ErrorCode` |
| `StackFrame` | 栈帧信息：`addr`, `symbol`, `file`, `line`, `col`, `offset` |
| `Breadcrumb` | 诊断面包屑：`timestamp_ms`, `category`, `message`, `level` |
| `MetadataEntry` | 诊断元数据键值对：`key`, `value` |
| `CrashReport` | 完整崩溃报告结构体（支持只读切片视图与 `CrashReportBuilder` 托管释放 `deinit`） |
| `CrashReportBuilder` | 基于 `std.ArrayList` 的报告构建器 |
| `FixedCrashWriter(N)` | 固定容量 `N` 字节零堆格式化写入器，溢出时安全截断并置位 `truncated` |
| `BreadcrumbRing(CAP)` | 固定容量 `CAP` 条目的零堆环形面包屑缓冲区，支持按时间顺序回放 |

## 3. API 速查

- **信号与原因映射**：
  - `signalName(sig: SignalKind): []const u8`
  - `signalDescription(sig: SignalKind): []const u8`
  - `signalFromCode(code: i32): SignalKind`
  - `reasonName(reason: CrashReason): []const u8`
  - `reasonSignal(reason: CrashReason): SignalKind`
  - `reasonAddress(reason: CrashReason): usize?`
- **零堆格式化与缓冲**：
  - `FixedCrashWriter(N).init()` / `writeBytes` / `writeHex` / `writeUint` / `writeInt` / `writeFrame` / `writeSignalSummary` / `writeRedactedEnv` / `isTruncated` / `slice` / `reset`
  - `BreadcrumbRing(CAP).init()` / `push` / `len` / `getFromOldest` / `iterFromOldest` / `clear`
- **报告导出**：
  - `formatCrashTextFixed(N, report, writer)`
  - `formatCrashText(report, alloc): ![]u8`
  - `formatCrashJson(report, alloc): ![]u8`

## 4. 示例

```cp
import {
    CrashReason,
    StackFrame,
    CrashReport,
    FixedCrashWriter,
    formatCrashTextFixed,
} from "cap:crash_handler";

pub fn main() {
    let frames: [1]StackFrame = [
        StackFrame{
            .addr = 0x4012a0,
            .symbol = "worker_loop",
            .file = "src/worker.cp",
            .line = 42,
            .col = 9,
            .offset = 0x10,
        },
    ];
    let report = CrashReport.init(
        CrashReason{ .SegmentationFault = 0xdeadbeef },
        0xdeadbeef,
        1001,
        1700000000000,
        frames[0..1],
        [],
        [],
    );
    let writer = FixedCrashWriter(1024).init();
    formatCrashTextFixed(1024, &report, &writer);
}
```

## 5. CPU 指令集特性诊断 (`cpuFeatures.cp`)

吸收自 `/data/dev/bun/src/crash_handler/CPUFeatures.cp`（参考 `/data/dev/bun/docs/installation.mdx#cpu-requirements`）：

- **导出符号**：
  - 常量与错误集：`CPU_FEATURES_FORMAT_BUF_CAP` (`64`)、`X86_BIT_SSE42` / `X86_BIT_POPCNT` / `X86_BIT_AVX` / `X86_BIT_AVX2` / `X86_BIT_AVX512`、`AARCH64_BIT_NEON` / `AARCH64_BIT_FP` / `AARCH64_BIT_AES` / `AARCH64_BIT_CRC32` / `AARCH64_BIT_ATOMICS` / `AARCH64_BIT_SVE`、`CpuFeaturesError`（`BufferTooSmall`/`UnknownFeatureToken`）
  - 类型：`CpuArchKind`（`X86_64`/`Aarch64`）、`X86CpuFlags`（`fromMask`/`toMask`/`isEmpty`/`hasAnyAvx`/`meetsBaseline`/`meetsHaswell`/`formatToBuf`/`parseFromStr`/`eql`）、`Aarch64CpuFlags`（`fromMask`/`toMask`/`isEmpty`/`hasCryptoAndCrc`/`formatToBuf`/`parseFromStr`/`eql`）、`CpuFeatures`（`fromX86`/`fromAarch64`/`fromRawMask`/`toRawMask`/`isEmpty`/`hasAnyAvx`/`formatToBuf`）
- **CPU 架构要求与崩溃诊断最佳实践（来自 Bun 官方文档）**：
  - **标准 x64 二进制**目标为 Haswell 架构，要求 `sse42 + popcnt + avx + avx2`（`meetsHaswell()`）；在无 AVX2 的老旧 CPU 上运行非 baseline 产物会触发 `SIGILL`（非法指令）；
  - **x64-baseline 二进制**目标为 Nehalem 架构，最低强制要求 `sse42 + popcnt`（`meetsBaseline()`）；
  - 在崩溃报告元数据中零分配调用 `CpuFeatures.formatToBuf(&buf)` 可输出空格分隔的指令集摘要（如 `"sse42 popcnt avx avx2"` 或 `"neon fp aes crc32 atomics"`），快速排查跨机非法指令故障。

```cp
import { CpuArchKind, CpuFeatures } from "cap:crash_handler";

const feats = CpuFeatures.fromRawMask(CpuArchKind.X86_64, (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4));
let buf: [64]u8 = [0; 64];
const n = feats.formatToBuf(buf[0..])?; // "sse42 popcnt avx avx2"
* = n;
```
