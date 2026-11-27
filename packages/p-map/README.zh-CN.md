[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查手册](README_AGENT.md)

# p-map

面向 CP 的零堆内存有界并发映射器（`pMap` / `pMapCollectErrors`）、跳过与错误结果协议（`mapOk`/`mapSkip`/`mapErr`）、批次/工作线程切片划分器以及 `ConcurrencyPool` 并发池状态机。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| **上游源平台** | [npmjs.com/package/p-map](https://www.npmjs.com/package/p-map) |
| **上游原包名** | `p-map`（CP 版本 `1.0.0`） |
| **上游仓库** | https://github.com/sindresorhus/p-map |
| **许可证** | MIT |

从 TypeScript/npm 迁移到 CP 保持原包名不变：

```ts
// TypeScript (npm)
import pMap, { pMapSkip } from "p-map";
const result = await pMap(items, mapper, { concurrency: 4, stopOnError: false });
```

```cp
// CP —— 原包名直接导入，零堆确定性并发映射与调度
import {
    pMap, pMapCollectErrors, defaultPMapOptions,
    mapOk, mapSkip, mapErr, ConcurrencyPool,
} from "p-map";
```

## 安装

```bash
cap add p-map
```

## 实战示例

### 1. 有界映射、跳过与错误聚合（`pMap` & `pMapCollectErrors`）

```cp
import {
    pMap, pMapCollectErrors, defaultPMapOptions,
    mapOk, mapSkip, mapErr,
    PMapOptions, PMapErrorEntry, MapOutcome,
} from "p-map";

fn transform(val: i64, _idx: usize): MapOutcome {
    if (val < 0) {
        return mapErr(-1);       // 记录错误码
    }
    if (val == 0) {
        return mapSkip();        // 等价于 npm pMapSkip
    }
    return mapOk(val * 10);
}

fn demoPMap() {
    let input: [5]i64 = [2, 0, -3, 4, -5];
    mut out: [5]i64 = [0; 5];
    mut errs: [5]PMapErrorEntry = [PMapErrorEntry { index: 0, err_code: 0 }; 5];

    // 收集全部错误而不提前中止（stop_on_error = false）
    let opts = PMapOptions { concurrency: 2, stop_on_error: false };
    let summary = pMapCollectErrors(&input, &mut out, &mut errs, opts, transform);
    // summary.produced == 2（out[0..2] = [20, 40]）
    // summary.skipped == 1
    // summary.failed == 2（errs[0].index == 2, errs[1].index == 4）
    // summary.halted == false

    // 快速失败模式（defaultPMapOptions 默认 stop_on_error = true）
    let strict_opts = defaultPMapOptions();
    let strict_summary = pMap(&input, &mut out, strict_opts, transform);
    // strict_summary.halted == true, strict_summary.first_error_index == 2
}
```

### 2. 批次与 Worker 区间划分（`batchCount` / `batchRange` / `workerPartition`）

```cp
import { batchCount, batchRange, workerPartition } from "p-map";

fn demoPartition() {
    let batches = batchCount(10, 3); // 4 批：[0..3), [3..6), [6..9), [9..10)
    if (batchRange(10, 3, 3)) |last_batch| {
        // last_batch.start == 9, last_batch.end == 10
    }

    // 将 10 个元素均匀分配给 4 个 worker
    if (workerPartition(10, 4, 0)) |slice0| {
        // Worker 0 处理 [slice0.start .. slice0.end)
    }
}
```

### 3. 有状态并发池（`ConcurrencyPool`）

```cp
import { ConcurrencyPool } from "p-map";

fn demoPool() {
    mut pool = ConcurrencyPool.new(10, 3, true);
    while (!pool.isDone()) {
        if (pool.acquireNext()) |idx| {
            if (idx % 2 == 0) {
                let _ = pool.reportOk();
            } else {
                let _ = pool.reportSkip();
            }
        }
    }
    let done = pool.completedCount();
    let skipped = pool.skippedCount();
    pool.reset();
}
```

## 性能与安全亮点

- **零堆分配**：映射输出与错误记录均写入调用方预分配切片（`out: []i64`、`errors_out: []PMapErrorEntry`），零 GC、零堆分配。
- **双错误策略**：同时支持遇错即停（`stop_on_error: true`）与全量错误聚合（`pMapCollectErrors` + `stop_on_error: false`）。
- **显式结果协议**：通过 `mapOk`、`mapSkip`、`mapErr` 明确区分成功、跳过与失败，避免魔法哨兵值冲突。
- **越界安全划分**：`batchRange` 与 `workerPartition` 在参数非法或索引越界时返回 `null`，不触发除零或越界。

## API 参考

完整类型定义见 [`src/index.d.cp`](src/index.d.cp)，AI 智能体极简速查见 [`README_AGENT.md`](README_AGENT.md)。
