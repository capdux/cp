# `@cap/traceparent`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:opentelemetry` ([https://github.com/open-telemetry/opentelemetry-rust](https://github.com/open-telemetry/opentelemetry-rust))（原库名 `opentelemetry` → CP 包名 `@cap/traceparent`）

Zero-heap W3C Trace Context (`traceparent` and `tracestate`) distributed tracing header parser, validator, child span builder, and formatter in pure CP. Strictly enforces lowercase hex, non-zero trace/parent IDs, and version rules per W3C Trace Context Level 1.

零堆分配纯 CP W3C Trace Context（`traceparent` 与 `tracestate`）分布式链路追踪头解析、校验与构造器，对标 OpenTelemetry，适用于微服务网关与 AI Agent 跨节点追踪。

## Installation

```bash
cap add @cap/traceparent
```

## API

| Function / Method | Description |
| --- | --- |
| `parseTraceParent(header)` | Strict W3C `traceparent` parser (`00-<32hex>-<16hex>-<2hex>`) |
| `TraceParent.isSampled()` | Check `0x01` sampled flag |
| `TraceParent.format(dst)` | Serialize back to exact 55-byte lowercase hex header |
| `makeChild(parent, new_span_id)` | Derive child span preserving `trace_id` and `trace_flags` |
| `getTraceState(header, vendor_key)` | Zero-copy vendor value lookup in `tracestate` |

## License

MIT
