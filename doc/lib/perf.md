# `cap:perf` / `cap.perf` — 性能剖析、作用域计时与 Trace Event 导出

## 1. 概述

`lib/perf/`（对外协议 `cap:perf` 与动态挂载 `cap.perf`）提供纯 CP 编写的高精度性能剖析、嵌套作用域计时、热点统计聚合（含 `P95`/`P99`）以及 Chrome Trace Event JSON（`chrome://tracing` / Perfetto）导出能力。支持系统单调纳秒时钟与确定性虚拟时钟注入，便于无抖动单元测试与生产级性能监控。

## 2. 核心类型

- **`PerfClock` / `PerfTimer`**：时间源抽象与计时器，支持 `Monotonic`（`osMonotonicNs()`）、`Virtual`（`advance` / `setNs`）与 `Custom` 回调时钟。
- **`PerfPhase`**：Trace 相位枚举：`Begin` (`'B'`)、`End` (`'E'`)、`Complete` (`'X'`)、`Instant` (`'i'`)、`Counter` (`'C'`)、`Metadata` (`'M'`)。
- **`PerfEvent`**：单条性能事件记录（`name`, `category`, `phase`, `ts_ns`, `dur_ns`, `pid`, `tid`, `value`）。
- **`PerfSpan`**：活动作用域句柄（含 `start_ns`, `depth`, `active`），用于严格的 `begin`/`end` 配对校验。
- **`PerfRecorder` / `PerfRecorderOptions`**：线程/进程上下文事件记录器，支持容量上限（`max_events`、`max_open_spans`）与空名称拦截。
- **`PerfStatSummary`**：按事件名聚合的统计结果（`count`, `total_ns`, `min_ns`, `max_ns`, `avg_ns`, `p95_ns`, `p99_ns`）。

## 3. API 速查

| 符号 | 说明 |
| --- | --- |
| `PerfClock.monotonic()` / `PerfClock.virtual(initial_ns)` | 创建单调时钟或确定性虚拟时钟 |
| `PerfClock.advance(delta_ns)` / `nowNs()` | 推进虚拟时钟 / 读取当前纳秒时间戳 |
| `PerfRecorder.init(alloc, clock, options)` / `deinit()` | 初始化与释放记录器 |
| `recorder.begin(name, category)` / `end(&span)` / `endByName(name)` | 作用域计时开始与配对结束（返回 `dur_ns`） |
| `recorder.recordComplete(name, cat, start_ns, dur_ns)` | 直接记录完整耗时事件 (`X`) |
| `recorder.recordInstant(name, cat)` / `recordCounter(name, cat, val)` | 记录瞬时事件 (`i`) 与计数器事件 (`C`) |
| `summarizeByName(alloc, events)` / `freeSummaries(alloc, summaries)` | 按名称聚合统计并计算 P95/P99 |
| `exportChromeTraceJson(alloc, events)` | 导出标准 `{"traceEvents":[...]}` JSON（RFC 8259 转义） |
| `formatSummaryTable(alloc, summaries)` | 格式化制表符分隔的统计摘要文本 |

## 4. 示例

```cp
import std from "std";
import { PerfClock, PerfRecorder, PerfRecorderOptions, summarizeByName, freeSummaries, exportChromeTraceJson } from "cap:perf";

pub fn runExample(alloc: std.mem.Allocator): !void {
    let rec = PerfRecorder.init(alloc, PerfClock.virtual(1_000_000), PerfRecorderOptions{});
    defer rec.deinit();

    let span = try rec.begin("compile", "frontend");
    rec.clock.advance(250_000);
    * = try rec.end(&span);

    const summaries = try summarizeByName(alloc, rec.items());
    defer freeSummaries(alloc, summaries);

    const trace_json = try exportChromeTraceJson(alloc, rec.items());
    defer alloc.free(trace_json);
}
```
