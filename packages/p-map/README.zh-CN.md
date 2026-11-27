[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# p-map

适用于 CP 语言的零堆分配受限并发映射器、批次/工作线程分区器与错误收集器，对齐 npm [`p-map`](https://github.com/sindresorhus/p-map)（MIT 协议，CP v1.0.0）。

提供支持 `stop_on_error` 策略与跳过哨兵（`mapSkip` / `pMapSkip`）的 `pMap` / `pMapCollectErrors`、可驱动自定义 Actor 或异步任务的 `ConcurrencyPool` 并发池状态机，以及零分配区间划分工具（`batchCount`、`batchRange`、`workerPartition`）。

## 安装与导入

```bash
cap add p-map
```

```cp
import {
    PMapOptions,
    MapOutcome,
    PMapErrorEntry,
    PMapSummary,
    ConcurrencyPool,
    defaultPMapOptions,
    mapOk,
    mapSkip,
    mapErr,
    pMap,
    pMapCollectErrors,
    batchCount,
    batchRange,
    workerPartition,
} from "p-map";
```

## 核心 API 参考

### 常量与数据结构

| 符号 | 字段 / 取值 | 说明 |
| :--- | :--- | :--- |
| `PMAP_STATUS_OK` / `SKIP` / `ERR` | `0` / `1` / `2` | `MapOutcome.status` 状态码常量。 |
| `BatchSlice` | `start: usize`, `end: usize` | 半开索引区间 `[start, end)`。 |
| `PMapOptions` | `concurrency: usize`, `stop_on_error: bool` | 并发度上限与遇错即停选项（`defaultPMapOptions()` 默认为 `8`、`true`）。 |
| `PMapSkip` | `marker: u8` | 对齐 npm `pMapSkip` 的跳过标记结构体（由 `pMapSkip()` 构造）。 |
| `MapOutcome` | `status: u8`, `value: i64`, `err_code: i32` | 映射回调返回值，通过 `mapOk(v)`、`mapSkip()`、`mapErr(code)` 构造。 |
| `PMapErrorEntry` | `index: usize`, `err_code: i32` | 单项错误记录，由 `pMapCollectErrors` 填充。 |
| `PMapSummary` | `total`, `produced`, `skipped`, `failed`, `batches_run`, `halted`, `first_error_index: usize?`, `first_error_code: i32` | `pMap` 与 `pMapCollectErrors` 返回的完整执行统计摘要。 |

### 函数与 `ConcurrencyPool`

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `pMap` | `(items: &[]i64, out: []i64, opts: PMapOptions, mapper: (i64, usize) => MapOutcome): PMapSummary` | 按受限并发批次映射 `items` 写入 `out`，自动忽略 `mapSkip()` 条目。 |
| `pMapCollectErrors` | `(items: &[]i64, out: []i64, errors_out: []PMapErrorEntry, opts: PMapOptions, mapper: (i64, usize) => MapOutcome): PMapSummary` | 同 `pMap`，同时将失败项索引与错误码记录到 `errors_out`。 |
| `batchCount` | `(total_items: usize, concurrency: usize): usize` | 计算总条目按给定并发度需拆分的批次数。 |
| `batchRange` | `(total_items: usize, batch_size: usize, batch_idx: usize): BatchSlice?` | 计算第 `batch_idx` 批的 `[start, end)` 区间，越界返回 `null`。 |
| `workerPartition` | `(total_items: usize, worker_count: usize, worker_idx: usize): BatchSlice?` | 将 `total_items` 均衡分片给 `worker_count` 个工作单元。 |
| `ConcurrencyPool.new` | `(total_items: usize, max_concurrency: usize, stop_on_error: bool): ConcurrencyPool` | 创建栈分配受限并发调度状态机。 |
| `ConcurrencyPool` 方法 | `acquireNext(): usize?`, `reportOk(): bool`, `reportSkip(): bool`, `reportErr(): bool`, `isDone(): bool`, `inFlight()`, `completedCount()`, `skippedCount()`, `failedCount()`, `remainingCount()`, `reset()` | 管理在途槽位领取、完成/跳过/错误记账及遇错中止状态。 |

## 代码示例

### 1. 带跳过项的映射（`pMap`）

```cp
import { defaultPMapOptions, mapOk, mapSkip, pMap, MapOutcome } from "p-map";

fn doublePositive(v: i64, idx: usize): MapOutcome {
    if (v <= 0) {
        return mapSkip();
    }
    return mapOk(v * 2);
}

fn runMap(): usize {
    let items: [4]i64 = [10, -1, 20, 30];
    let out: [4]i64 = undefined;
    let summary = pMap(&items, out[0..4], defaultPMapOptions(), doublePositive);
    // summary.produced == 3, summary.skipped == 1 -> out[0..3] 为 [20, 40, 60]
    return summary.produced;
}
```

### 2. 容错执行并收集全部错误（`pMapCollectErrors`）

```cp
import { PMapOptions, PMapErrorEntry, MapOutcome, mapOk, mapErr, pMapCollectErrors } from "p-map";

fn validateItem(v: i64, idx: usize): MapOutcome {
    if (v < 0) { return mapErr(-100); }
    return mapOk(v);
}

fn collectAll() {
    let items: [4]i64 = [1, -2, 3, -4];
    let out: [4]i64 = undefined;
    let errs: [4]PMapErrorEntry = undefined;
    let opts = PMapOptions { concurrency: 2, stop_on_error: false };
    let sum = pMapCollectErrors(&items, out[0..4], errs[0..4], opts, validateItem);
    // sum.produced == 2, sum.failed == 2, errs[0].index == 1, errs[1].index == 3
}
```

### 3. 使用 `ConcurrencyPool` 驱动自定义并发调度

```cp
import { ConcurrencyPool } from "p-map";

fn scheduleLoop() {
    let mut pool = ConcurrencyPool.new(10, 3, true);
    while (!pool.isDone()) {
        let next = pool.acquireNext();
        if (next != null) {
            // 派发任务索引 `next.?`，完成后回报状态：
            pool.reportOk();
        }
    }
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配**：映射输出与错误条目直接写入调用方切片（`out: []i64`、`errors_out: []PMapErrorEntry`），不动态分配数组或 `AggregateError`。
- **解耦并发状态机**：`ConcurrencyPool` 将并发槽位记账与底层运行时解耦，可无缝对接 CP Actor、事件循环或工作线程池。
- **结构化执行摘要**：`PMapSummary` 提供精确的 `produced`、`skipped`、`failed`、`batches_run`、`halted` 与 `first_error_index: usize?`。
