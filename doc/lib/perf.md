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

## 5. 进程与子进程资源用量剖析 (`resourceUsage.cp`)

吸收自 `/data/dev/bun/src/runtime/api/bun/subprocess/ResourceUsage.cp`（参考 `/data/dev/bun/docs/runtime/child-process.mdx` 中 `proc.resourceUsage()` 实战指南）：

- **导出符号**：`RUSAGE_USEC_PER_SEC` (`1_000_000`)、`RUSAGE_BYTES_PER_KB` (`1_024`)、`ResourceUsageError`（`BufferTooSmall`）、`CpuTimeUs`（`fromTimevals`/`userSharePermille`）、`IoBlockOps`（`total`）、`IpcMessages`（`total`）、`ContextSwitches`（`total`/`involuntaryRatioPermille`）、`ResourceUsage`（`linuxMaxRssKbToBytes`/`cpuTime`/`ops`/`messages`/`contextSwitches`/`diff`/`merge`/`formatSummary`）；
- **实战最佳实践**：
  - 子进程退出后（如 `await proc.exited` 后）采集 `ResourceUsage`，可获取峰值常驻内存（`max_rss_bytes`）、用户态/内核态微秒级 CPU 耗时（`cpuTime()`）、块 I/O 次数（`ops()`）、IPC 消息数（`messages()`）与上下文切换次数（`contextSwitches()`）；
  - Linux `getrusage(2)` 返回的 `ru_maxrss` 单位为 KiB，调用 `ResourceUsage.linuxMaxRssKbToBytes(kb)` 可与 macOS 统一为字节口径；
  - 使用 `diff(earlier)` 进行区间资源差分（饱和减法防倒退，RSS 取峰值），使用 `merge(other)` 聚合多 Worker/子进程集群总开销，调用 `formatSummary(&buf)` 零分配输出诊断摘要。

```cp
import { ResourceUsage } from "cap:perf";

const u = ResourceUsage{
    utime_sec: 0,
    utime_usec: 12_500,
    stime_sec: 0,
    stime_usec: 2_500,
    max_rss_bytes: ResourceUsage.linuxMaxRssKbToBytes(4096),
    in_block_ops: 8,
    out_block_ops: 2,
    voluntary_ctx_switches: 15,
    involuntary_ctx_switches: 3,
};
let buf: [128]u8 = [0; 128];
const n = u.formatSummary(buf[0..])?;
* = n;
```

## 6. 长生命周期服务内存账本与泄漏诊断 (`memoryCost.cp`)

吸收自 `/data/dev/bun/src/bake/DevServer/memory_cost.cp`（参考 `/data/dev/bun/docs/project/benchmarking.mdx` 内存剖析指南）：

- **导出符号**：`MemoryCostError`（`BufferTooSmall`）、`MemoryCostCategory`（`IncrementalGraphClient`/`IncrementalGraphServer`/`JsCode`/`SourceMaps`/`Assets`/`Other`）、`MemoryLeakVerdict`（`Healthy`/`SuspectedUntrackedLeak`/`UnboundedTrackedGrowth`）、`estimateSliceBytes`、`estimateCapacityBytes`、`MemoryCostBreakdown`（`totalBytes`/`add`/`bytesFor`/`sharePermille`/`largestCategory`/`merge`/`diffSigned`/`diagnoseVsRss`/`formatSummary`）；
- **内存泄漏双维度定位最佳实践（来自 Bun DevServer 工程经验）**：
  - 将服务显式持有的内存按 6 大类记账，定期与进程 `RSS`（如 `ResourceUsage.max_rss_bytes`）交叉比对：
  - 若显式记账总额保持平稳而进程 RSS 持续上涨（`diagnoseVsRss` 返回 `.SuspectedUntrackedLeak`），说明存在未追踪内存泄漏（如遗忘释放的底层缓冲或外部句柄）；
  - 若显式记账总额自身失控增长（返回 `.UnboundedTrackedGrowth`），说明增量缓存/淘汰或 GC 策略不足，可结合 `largestCategory()` 与 `sharePermille()` 立即定位最大增长源。

```cp
import { MemoryCostBreakdown, MemoryCostCategory, MemoryLeakVerdict } from "cap:perf";

let t0 = MemoryCostBreakdown{ js_code: 4096, source_maps: 2048 };
let t1 = t0;
t1.add(MemoryCostCategory.Assets, 1024);
const verdict = t1.diagnoseVsRss(t0, 10_000_000, 25_000_000, 4_000_000);
// verdict == MemoryLeakVerdict.SuspectedUntrackedLeak
* = verdict;
```
