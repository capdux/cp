# CP 标准库：纳秒级硬件计时器、基准统计与对数延迟直方图使用手册 (`cap:bench`)

## 一、 架构总览与设计哲学

`cap:bench` 是 CP 语言内置的**零堆分配、纳秒级性能基准测试与延迟观测子系统**，吸收自 Bun 的性能剖析模块（`/data/dev/bun/src/perf/hw_timer.cp` 与 `perf.cp`）。全部数据结构均为栈上固定布局，测量过程不引入任何动态内存分配，避免观测工具本身干扰微基准（Microbenchmark）结果。

```text
┌──────────────────────────────────────────────────────────────────────────────┐
│                        cap:bench 性能剖析与统计分层                           │
│                                                                              │
│  ┌──────────────────────────────┐     ┌───────────────────────────────────┐  │
│  │ L1 · 硬件单调时钟原语         │     │ L2 · 高层计时器                   │  │
│  │  • HwTimer.nowNs()           │ ──▶ │  • Stopwatch.start()              │  │
│  │  • HwTimer.nowUs() / nowMs() │     │  • readNs / readUs / readMs       │  │
│  │  (CLOCK_MONOTONIC, 零回拨)    │     │  • lap() / reset()                │  │
│  └──────────────────────────────┘     └─────────────────┬─────────────────┘  │
│                                                         ▼                    │
│  ┌───────────────────────────────────────┐ ┌──────────────────────────────┐  │
│  │ L3-A · 精确小样本分位数统计            │ │ L3-B · 无限样本对数直方图     │  │
│  │  • BenchStats (128 槽插入排序)        │ │  • LatencyHistogram (8 档)   │  │
│  │  • meanNs / p50Ns / p95Ns / p99Ns    │ │  • <100ns … >=100ms          │  │
│  │  • opsPerSec() 吞吐估算               │ │  • estimatePercentileBoundNs │  │
│  └───────────────────────────────────────┘ └──────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、 核心 API 详解与代码示例

### 1. 硬件单调时钟 (`HwTimer`) 与秒表 (`Stopwatch`)

基于 Linux `CLOCK_MONOTONIC`，不受系统墙上时钟（NTP/手动校时）跳变影响：

```typescript
import { HwTimer, Stopwatch } from "cap:bench";

pub fn demoTimer() {
    const t0 = HwTimer.nowNs();
    let sw = Stopwatch.start();

    // ... 执行待测逻辑第一阶段 ...
    const stage1_ns = sw.lap();

    // ... 执行待测逻辑第二阶段 ...
    const stage2_ns = sw.lap();
    const total_us = sw.readUs();

    sw.reset();
    const elapsed_ms = HwTimer.nowMs() - (t0 / 1000000);
}
```

### 2. 精确分位数与吞吐统计 (`BenchStats`)

`BenchStats` 在栈内维护 128 个有序纳秒样本（插入时维持升序），实现 $O(1)$ 查询 `p50`、`p95`、`p99` 与 `ops/s`：

```typescript
import { Stopwatch, BenchStats } from "cap:bench";

pub fn demoBenchStats() {
    let stats = BenchStats.init();

    let i: usize = 0;
    while (i < 100) : (i += 1) {
        let sw = Stopwatch.start();
        // ... 执行单次操作 ...
        stats.record(sw.readNs());
    }

    const avg = stats.meanNs();
    const p50 = stats.p50Ns();
    const p95 = stats.p95Ns();
    const p99 = stats.p99Ns();
    const throughput = stats.opsPerSec();
}
```

### 3. 8 档对数延迟直方图 (`LatencyHistogram`)

当采样次数达到数百万次（如高频 RPC / HTTP 服务端延迟监控）时，使用 `LatencyHistogram` 可在恒定内存（仅 160 字节）下以 $O(1)$ 记录任意数量样本：

| 桶索引 | 覆盖区间 | 桶上界 (`bounds[i]`) |
|---|---|---|
| `0` | `< 100 ns` | `100` |
| `1` | `100 ns .. < 1 µs` | `1,000` |
| `2` | `1 µs .. < 10 µs` | `10,000` |
| `3` | `10 µs .. < 100 µs` | `100,000` |
| `4` | `100 µs .. < 1 ms` | `1,000,000` |
| `5` | `1 ms .. < 10 ms` | `10,000,000` |
| `6` | `10 ms .. < 100 ms` | `100,000,000` |
| `7` | `>= 100 ms` | `max_ns` |

```typescript
import { LatencyHistogram } from "cap:bench";

pub fn demoHistogram() {
    let hist = LatencyHistogram.init();
    hist.record(42);
    hist.record(850);
    hist.record(12500);
    hist.record(450000);

    const total = hist.count();                         // 4
    const mean = hist.meanNs();
    const p99_bound = hist.estimatePercentileBoundNs(99);
    const sub_1us = hist.bucketCount(0) + hist.bucketCount(1);
    hist.reset();
}
```

---

## 三、 与 Bun / Node.js 性能 API 对照表

| 能力 | Node.js / Bun | CP (`cap:bench`) | 优势说明 |
|---|---|---|---|
| 纳秒单调时钟 | `Bun.nanoseconds()` / `process.hrtime.bigint()` | `HwTimer.nowNs()` | 返回原生 `u64`，无 BigInt 装箱与 GC 开销 |
| 分段计时器 | 手动相减 `performance.now()` | `Stopwatch.start()` / `lap()` / `readNs()` | 封装单调防倒退差值，API 简洁直观 |
| 基准统计 | 第三方 `mitata` / `tinybench` | 内置 `BenchStats` | 零堆分配栈内排序，直接提供 `p50/p95/p99/opsPerSec` |
| 延迟直方图 | `perf_hooks.createHistogram()` | `LatencyHistogram` | 固定 160B 栈结构，$O(1)$ 记录，适合微秒/纳秒级热路径 |
