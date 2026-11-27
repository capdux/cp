[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# opentelemetry

Zero-heap W3C Trace Context (traceparent & tracestate) distributed tracing header parser and formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:opentelemetry` ([https://github.com/open-telemetry/opentelemetry-rust](https://github.com/open-telemetry/opentelemetry-rust))（原库名 `opentelemetry` → CP 包名 `opentelemetry`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `opentelemetry` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add opentelemetry
```

```cp
import {
    TraceParent,
    parseHexByteLower,
    invalidTraceParent,
    parseTraceParent,
    makeChild,
    getTraceState,
} from "opentelemetry";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TraceParent` | `class` | 字段: `version: u8, trace_id: [16]u8, parent_id: [8]u8, trace_flags: u8, valid: bool` · 方法: `isSampled, format` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `parseHexByteLower` | `(hi: u8, lo: u8): i32` | 通过 `parseHexByteLower` 解析输入数据，返回 `i32`。 |
| `invalidTraceParent` | `(): TraceParent` | 调用 `invalidTraceParent` 并返回 `TraceParent`。 |
| `parseTraceParent` | `(header: &[]u8): TraceParent` | 通过 `parseTraceParent` 解析输入数据，返回 `TraceParent`。 |
| `makeChild` | `(parent: TraceParent, new_span_id: [8]u8): TraceParent` | 调用 `makeChild` 并返回 `TraceParent`。 |
| `getTraceState` | `(header: &[]u8, vendor_key: &[]u8): &[]u8` | 查询或读取目标数据，返回 `&[]u8`。 |

## 最小调用示例

```cp
import { parseHexByteLower } from "opentelemetry";

pub fn main() {
    let result = parseHexByteLower(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
