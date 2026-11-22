# CP 标准库：纳秒级计时器、微基准统计对比与内存剖析手册 (`cap:bench`)

> 参考来源：`/data/dev/bun/src/perf/hw_timer.cp`, `/data/dev/bun/src/perf/perf.cp`, `/data/dev/bun/src/perf/tracy.cp` | 吸收日期：2026-11-22
> **模块路径**: `lib/bench/`  
> **导入协议**: `import { HwTimer, Stopwatch, BenchStats, LatencyHistogram, TracePhase, TraceEvent, TraceRingBuffer, PerfTrace, BenchCaseResult, BenchSuite, blackBox, BenchOptions, BenchResult, BenchRunner, BenchVerdict, BenchComparison, BenchTableFormatter, AllocSnapshot, CountingAllocator } from "cap:bench";`

---

## 一、 架构总览与核心分层

`cap:bench` 整合自 Bun `src/perf/` 性能剖析引擎与工业生产级微基准统计对比/内存追踪套件，全部核心运行期路径均采用**零额外干扰与低开销布局**：

1. **硬件单调时钟原语 (`HwTimer` & `Stopwatch`)**：基于 `CLOCK_MONOTONIC` 的纳秒/微秒/毫秒单调计时与分段 `lap()` 测量。
2. **完整分位数与 Welford 方差统计 (`BenchStats`)**：
   - 维护最多 512 个有序样本及 Welford 在线方差状态；
   - 提供 `minNs` / `maxNs` / `meanNs` / `medianNs` (`p50Ns`) / `p75Ns` / `p90Ns` / `p95Ns` / `p99Ns` / `p999Ns` / `stdDevNs` / `stdDevNsF64` / `opsPerSec` / `opsPerSecF64`。
3. **8 档对数桶延迟直方图 (`LatencyHistogram`)**：恒定栈内存记录任意规模样本，支持 `<100ns` .. `>=100ms` 分桶与分位上界估算。
4. **零堆分配 Perf 埋点事件环 (`TraceRingBuffer<CAP>` / `PerfTrace`)**：
   - 支持 `begin` / `end` / `endSpan` / `instant` / `complete` 四类 Trace 阶段；
   - 支持直接导出标准 Chrome Trace Event JSON（`exportChromeTraceJson`），可在 `chrome://tracing` 或 Perfetto 中可视化火焰图。
5. **微基准测试运行器与优化屏障 (`blackBox`, `BenchOptions`, `BenchResult`, `BenchRunner`, `BenchSuite`)**：
   - `blackBox<T>(val: T): T`：封装 `std.mem.doNotOptimizeAway`，阻止编译器对基准载荷执行死代码消除；
   - `BenchRunner.runSamples(name, durations_ns)`：支持外部样本注入，用于确定性统计与分位数（`p50_ns` / `p95_ns` / `p99_ns`、`mean_ns`、`stddev_ns`、`ops_per_sec`）计算；
   - `BenchRunner.measure(name, opts, fn_ptr)`：按 `BenchOptions`（`warmup_iters`、`sample_count`、`min_sample_ns`）自动预热与自适应批次数校准，输出 `BenchResult`；
   - 保留多用例静态缓冲基准套件 `BenchSuite`（支持 `toMarkdown` / `toJson`）100% 向后兼容。
6. **Welch t-test 统计对比与 Markdown 报告 (`BenchVerdict`, `BenchComparison`, `BenchTableFormatter`)**：
   - `BenchComparison.compare(baseline, candidate, threshold_pct)`：计算相对变化百分比、加速比、Welch's t-statistic、Welch-Satterthwaite 自由度近似及 95% 置信度显著性检验，自动裁决 `Faster` / `Slower` / `NoChange`；
   - `BenchTableFormatter.formatMarkdown(alloc, results)` 与 `formatComparison(alloc, cmp)`：输出标准 GitHub Flavored Markdown 性能表格与对比报告。
7. **内存分配计数剖析器 (`AllocSnapshot`, `CountingAllocator`)**：
   - 零侵入包装任意 `std.mem.Allocator`；
   - 精确追踪 `alloc_count`、`free_count`、`realloc_count`、`total_bytes_allocated`、`live_bytes` 与 `peak_bytes`，提供 `snapshot()` 与 `reset()` 用于单次迭代泄漏与峰值内存回归检测。

---

## 二、 实战代码示例

### 1. `BenchRunner`、`blackBox` 与 `BenchComparison` (Welch's t-test)

```typescript
const std = @import("std");
import {
    blackBox,
    BenchOptions,
    BenchResult,
    BenchRunner,
    BenchVerdict,
    BenchComparison,
    BenchTableFormatter,
} from "cap:bench";

fn workV1() {
    let s: u64 = 0;
    for (i of 0..128) { s +%= i; }
    * = blackBox(s);
}

fn workV2() {
    let s: u64 = 0;
    for (i of 0..64) { s +%= i * 2; }
    * = blackBox(s);
}

pub fn demoCompare(alloc: std.mem.Allocator): ! {
    const opts = BenchOptions {
        warmup_iters: 5,
        sample_count: 30,
        min_sample_ns: 100000,
    };
    const r1 = BenchRunner.measure("work_v1", opts, workV1);
    const r2 = BenchRunner.measure("work_v2", opts, workV2);

    const cmp = BenchComparison.compare(r1, r2, 5.0);
    if (cmp.verdict == BenchVerdict.Faster) {
        const md_cmp = BenchTableFormatter.formatComparison(alloc, cmp)?;
        defer alloc.free(md_cmp);
    }

    const all: [2]BenchResult = [r1, r2];
    const md_table = BenchTableFormatter.formatMarkdown(alloc, &all)?;
    defer alloc.free(md_table);
}
```

### 2. `CountingAllocator` 单次基准堆分配与峰值追踪

```typescript
const std = @import("std");
import { CountingAllocator } from "cap:bench";

pub fn demoMemoryBench(): ! {
    let tracker = CountingAllocator.init(std.heap.page_allocator);
    const alloc = tracker.allocator();

    const buf = alloc.alloc(u8, 256)?;
    const mid = tracker.snapshot(); // live_bytes == 256, peak_bytes == 256
    alloc.free(buf);

    const end = tracker.snapshot(); // live_bytes == 0, peak_bytes == 256, hasLeaks() == false
    tracker.reset();                // 清零计数，准备下一轮迭代
}
```

### 3. `BenchSuite` 与 `PerfTrace`（向后兼容套件）

```typescript
import { PerfTrace, BenchSuite } from "cap:bench";

fn work() {
    let s: u64 = 0;
    for (i of 0..64) { s +%= i; }
}

pub fn demoLegacy() {
    let suite = BenchSuite.init("HotPath Bench", 5, 50, 1000000, 10000);
    suite.run("sum_64", work);

    let trace = PerfTrace.init();
    const span = trace.begin("parse", "compiler");
    trace.instant("lex_done", "compiler");
    * = trace.endSpan(span);
}
```

---

## 上游 Bun `perf` 行为契约与基准测量注意事项

1. **防止死代码消除（`blackBox`）**：
   - 在 Release 优化模式下，未消费的纯计算结果会被 LLVM 完全消除导致“0 ns 假象”；微基准回调的输入与返回值必须包裹 `blackBox(val)` 强制建立优化屏障。
2. **Welch t-test 显著性对比阈值（`BenchComparison.compare`）**：
   - 仅凭单次均值对比极易受系统噪声误导；`BenchComparison.compare(baseline, candidate, threshold_pct)` 结合 Welford 方差与 Welch-Satterthwaite 自由度做 95% 置信度检验，噪声过大时稳健裁决为 `BenchVerdict.NoChange`。
3. **零干扰内存计数（`CountingAllocator`）与火焰图导出**：
   - 基准循环外包装 `CountingAllocator` 可精确观测单次调用的分配次数与峰值字节数；`TraceRingBuffer<CAP>` 固定容量环形记录埋点，满载时覆写最旧事件，支持导出 Chrome Trace JSON 导入 Perfetto 分析。
