[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# opentelemetry

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap W3C Trace Context (traceparent & tracestate) distributed tracing header parser and formatter in pure CP

## 安装与导入

```bash
cap add opentelemetry
```

```cp
import { hexValLower } from "opentelemetry";
```

## 核心导出 API

- `export fn hexValLower(c: u8): i32;`
- `export fn parseHexByteLower(hi: u8, lo: u8): i32;`
- `export fn invalidTraceParent(): TraceParent;`
- `export fn parseTraceParent(header: &[]u8): TraceParent;`
- `export fn makeChild(parent: TraceParent, new_span_id: [8]u8): TraceParent;`
- `export fn getTraceState(header: &[]u8, vendor_key: &[]u8): &[]u8;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`opentelemetry@1.0.0`)，许可证：Apache-2.0
