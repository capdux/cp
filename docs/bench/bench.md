# CP 标准库：纳秒级计时器、分位数基准套件与 Chrome Trace 埋点手册 (`cap:bench`)

> **上游溯源**: `/data/dev/bun/src/perf/hw_timer.cp`, `/data/dev/bun/src/perf/perf.cp`, `/data/dev/bun/src/perf/tracy.cp`  
> **模块路径**: `lib/bench/`  
> **导入协议**: `import { HwTimer, Stopwatch, BenchStats, LatencyHistogram, TracePhase, TraceEvent, TraceRingBuffer, PerfTrace, BenchSuite } from "cap:bench";`

---

## 一、 架构总览与核心分层

`cap:bench` 吸收自 Bun `src/perf/` 性能剖析引擎，全部核心组件均采用**零堆分配固定布局**：

1. **硬件单调时钟原语 (`HwTimer` & `Stopwatch`)**：基于 `CLOCK_MONOTONIC` 的纳秒/微秒/毫秒单调计时与分段 `lap()` 测量。
2. **完整分位数与 Welford 方差统计 (`BenchStats`)**：
   - 维护最多 512 个有序样本及 Welford 在线方差状态；
   - 提供 `minNs` / `maxNs` / `meanNs` / `medianNs` (`p50Ns`) / `p75Ns` / `p90Ns` / `p95Ns` / `p99Ns` / `p999Ns` / `stdDevNs` / `stdDevNsF64` / `opsPerSec` / `opsPerSecF64`。
3. **8 档对数桶延迟直方图 (`LatencyHistogram`)**：恒定栈内存记录任意规模样本，支持 `<100ns` .. `>=100ms` 分桶与分位上界估算。
4. **零堆分配 Perf 埋点事件环 (`TraceRingBuffer<CAP>` / `PerfTrace`)**：
   - 吸收自 Bun `tracy.cp`，支持 `begin` / `end` / `endSpan` / `instant` / `complete` 四类 Trace 阶段；
   - 支持直接导出标准 Chrome Trace Event JSON（`exportChromeTraceJson`），可在 `chrome://tracing` 或 Perfetto 中可视化火焰图。
5. **多用例基准套件 (`BenchSuite`)**：
   - 支持配置预热次数（`warmup_iters`）、最小迭代（`min_iters`）与最小时长（`min_time_ns`）；
   - 支持导出 Markdown 对比表格（`toMarkdown`）与全量指标 JSON 报告（`toJson`）。

---

## 二、 实战代码示例

### 1. `BenchSuite` 与完整分位数统计

```typescript
import { BenchStats, BenchSuite } from "cap:bench";

fn work() {
    let s: u64 = 0;
    for (i of 0..64) { s +%= i; }
}

pub fn demoSuite() {
    let suite = BenchSuite.init("HotPath Bench", 5, 50, 1000000, 10000);
    suite.run("sum_64", work);

    let md_buf: [2048]u8 = undefined;
    const md = suite.toMarkdown(&md_buf);

    let json_buf: [2048]u8 = undefined;
    const json = suite.toJson(&json_buf);
}
```

### 2. `PerfTrace` / `TraceRingBuffer` 纳秒级埋点与 Chrome Trace 导出

```typescript
import { PerfTrace } from "cap:bench";

pub fn demoTrace() {
    let trace = PerfTrace.init();
    const span = trace.begin("parse", "compiler");
    trace.instant("lex_done", "compiler");
    * = trace.endSpan(span);

    let json_buf: [4096]u8 = undefined;
    const chrome_json = trace.exportChromeTraceJson(&json_buf);
}
```
