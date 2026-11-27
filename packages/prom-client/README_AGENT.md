# prom-client — AI Agent Quick Ref

- **Package**: `prom-client` (`1.0.0`) | **Source**: npm `prom-client` v15 (zero-heap Prometheus text exposition serializer)
- **Import**: `import { Label, PromWriter, isValidMetricName, isValidLabelName, METRIC_COUNTER, METRIC_GAUGE, METRIC_HISTOGRAM, METRIC_SUMMARY } from "prom-client";`
- **Allocation**: Zero-heap; formats directly into caller `[]u8` with sticky `overflow` detection.

## Constants & Core Signatures

```typescript
METRIC_COUNTER: u8 = 0; METRIC_GAUGE: u8 = 1; METRIC_HISTOGRAM: u8 = 2; METRIC_SUMMARY: u8 = 3;

export class Label { key: &[]u8, val: &[]u8 }

export fn isValidMetricName(name: &[]u8): bool;
export fn isValidLabelName(name: &[]u8): bool;

export class PromWriter {
    buf: []u8, pos: usize, overflow: bool,
    fn init(buf: []u8): PromWriter;
    fn writeHelp(_: auto, name: &[]u8, help: &[]u8): bool;
    fn writeType(_: auto, name: &[]u8, kind: u8): bool;
    fn writeSampleInt(_: auto, name: &[]u8, labels: &[]Label, value: i64): bool;
    fn writeSampleWithTs(_: auto, name: &[]u8, labels: &[]Label, value: i64, ts_ms: i64): bool;
    fn written(): &[]u8;
    fn len(): usize;
    fn isOverflow(): bool;
}
```

## Minimal Pattern

```typescript
import { Label, PromWriter, METRIC_COUNTER } from "prom-client";

let buf: [4096]u8 = undefined;
let w = PromWriter.init(buf[0..]);
_ = w.writeHelp("req_total", "Requests");
_ = w.writeType("req_total", METRIC_COUNTER);
const labels: [1]Label = [Label { key: "code", val: "200" }];
_ = w.writeSampleInt("req_total", labels[0..], 99);
const body = w.written();
```

## Pitfalls

1. **Check `w.isOverflow()`**: Once `buf` runs out of space, all subsequent `write*` calls return `false` and `overflow` stays `true`.
2. **Invalid names return `false`**: Metric names with `-` or spaces, or label names starting with `__`, fail validation and return `false`.
3. **Integer samples**: `writeSampleInt` and `writeSampleWithTs` take `i64` values (not floats).
4. **Kind constant range**: Pass `METRIC_COUNTER` (0), `METRIC_GAUGE` (1), `METRIC_HISTOGRAM` (2), or `METRIC_SUMMARY` (3) to `writeType`.
