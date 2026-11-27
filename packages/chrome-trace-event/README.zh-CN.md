[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# chrome-trace-event

Zero-heap Chrome DevTools / Perfetto Trace Event JSON serializer

| 元信息 | 值 |
| --- | --- |
| 包名 | `chrome-trace-event` |
| 版本 | `1.0.4` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add chrome-trace-event
```

```cp
import {
    TracePhase,
    TraceEvent,
    phaseCode,
    ctevWriteByte,
    ctevWriteStr,
    ctevWriteJsonStr,
    ctevWriteU64,
    formatTraceEvent,
} from "chrome-trace-event";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `TracePhase` | `Begin, End, Complete, Instant, Counter, Metadata` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TraceEvent` | `class` | 字段: `name: string, cat: string, ph: TracePhase, ts_us: u64, dur_us: u64, pid: u32, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `phaseCode` | `(ph: TracePhase): u8` | 调用 `phaseCode` 并返回 `u8`。 |
| `ctevWriteByte` | `(out: []u8, pos: i64, b: u8): i64` | 调用 `ctevWriteByte` 并返回 `i64`。 |
| `ctevWriteStr` | `(out: []u8, pos: i64, s: string): i64` | 调用 `ctevWriteStr` 并返回 `i64`。 |
| `ctevWriteJsonStr` | `(out: []u8, pos: i64, s: string): i64` | 调用 `ctevWriteJsonStr` 并返回 `i64`。 |
| `ctevWriteU64` | `(out: []u8, pos: i64, val: u64): i64` | 调用 `ctevWriteU64` 并返回 `i64`。 |
| `formatTraceEvent` | `(ev: TraceEvent, out: []u8): usize` | 通过 `formatTraceEvent` 编码或格式化输出，返回 `usize`。 |
| `formatTraceArray` | `(events: &[]TraceEvent, out: []u8): usize` | 通过 `formatTraceArray` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { ctevWriteStr } from "chrome-trace-event";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ctevWriteStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
