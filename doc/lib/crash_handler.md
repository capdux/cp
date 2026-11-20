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
