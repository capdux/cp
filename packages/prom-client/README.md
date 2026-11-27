[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# prom-client

Pure CP, zero-heap **Prometheus / OpenMetrics text exposition format** serializer with strict metric/label name validation, automatic `HELP` and label-value escaping, and overflow tracking.

## Provenance & Zero-Cost Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | [npm](https://www.npmjs.com/package/prom-client) |
| **Upstream Package** | `prom-client` (v15 parity) |
| **Upstream Repo** | https://github.com/siimon/prom-client |
| **CP Package** | `prom-client` (`1.0.0`) |
| **License** | MIT / Apache-2.0 |

Keep the exact same package name in CP — no renaming required:

```typescript
// TypeScript (npm)
import client from "prom-client";

// CP (cap)
import {
    Label,
    PromWriter,
    isValidMetricName,
    isValidLabelName,
    METRIC_COUNTER,
    METRIC_GAUGE,
    METRIC_HISTOGRAM,
    METRIC_SUMMARY,
} from "prom-client";
```

## Installation

```bash
cap add prom-client
```

## Key Features & Security Highlights

- **Zero Heap Allocation**: `PromWriter` formats Prometheus `/metrics` payloads directly into a caller-provided `[]u8` buffer — ideal for high-frequency scraping endpoints.
- **Strict Prometheus Identifier Validation**: `isValidMetricName` enforces `[a-zA-Z_:][a-zA-Z0-9_:]*`, and `isValidLabelName` enforces `[a-zA-Z_][a-zA-Z0-9_]*` while rejecting reserved `__*` prefixes.
- **Safe Exposition Escaping**: Automatically escapes `\`, `\n`, and `"` in `HELP` comments and label values to prevent exposition format injection.
- **Sticky Overflow Flag**: If the destination buffer fills up, `PromWriter` sets `overflow = true` and safely rejects subsequent writes without out-of-bounds memory access.

## Quick Start

```typescript
import {
    Label,
    PromWriter,
    METRIC_COUNTER,
    METRIC_GAUGE,
} from "prom-client";

fn formatMetrics(buf: []u8): &[]u8 {
    let w = PromWriter.init(buf);

    _ = w.writeHelp("http_requests_total", "Total HTTP requests processed");
    _ = w.writeType("http_requests_total", METRIC_COUNTER);

    const labels: [2]Label = [
        Label { key: "method", val: "GET" },
        Label { key: "status", val: "200" },
    ];
    _ = w.writeSampleInt("http_requests_total", labels[0..], 4215);

    _ = w.writeHelp("active_agents", "Currently active BEAM agents");
    _ = w.writeType("active_agents", METRIC_GAUGE);
    const empty_labels: [0]Label = [];
    _ = w.writeSampleWithTs("active_agents", empty_labels[0..], 1024, 1700000000000);

    if (w.isOverflow()) {
        // Caller buffer was too small
    }
    return w.written();
}
```

## API Overview

| Symbol | Signature / Value | Description |
| :--- | :--- | :--- |
| `METRIC_COUNTER` / `METRIC_GAUGE` / `METRIC_HISTOGRAM` / `METRIC_SUMMARY` | `0` / `1` / `2` / `3` | Metric type constants for `writeType` |
| `Label` | `class { key: &[]u8, val: &[]u8 }` | Metric label key-value pair |
| `isValidMetricName` | `(name: &[]u8) -> bool` | Validate Prometheus metric name syntax |
| `isValidLabelName` | `(name: &[]u8) -> bool` | Validate Prometheus label name (rejects `__*` reserved prefix) |
| `PromWriter.init` | `(buf: []u8) -> PromWriter` | Initialize zero-heap writer over `buf` |
| `PromWriter.writeHelp` | `(name: &[]u8, help: &[]u8) -> bool` | Emit `# HELP <name> <escaped_help>\n` |
| `PromWriter.writeType` | `(name: &[]u8, kind: u8) -> bool` | Emit `# TYPE <name> <counter\|gauge\|histogram\|summary>\n` |
| `PromWriter.writeSampleInt` | `(name: &[]u8, labels: &[]Label, value: i64) -> bool` | Emit metric sample line with integer value |
| `PromWriter.writeSampleWithTs` | `(name: &[]u8, labels: &[]Label, value: i64, ts_ms: i64) -> bool` | Emit metric sample line with value and millisecond timestamp |
| `PromWriter.written` | `() -> &[]u8` | Return formatted byte slice `buf[0..pos]` |
| `PromWriter.len` / `isOverflow` | `() -> usize` / `() -> bool` | Current written byte length and overflow state |

## License

MIT / Apache-2.0
