[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查手册](README_AGENT.md)

# hdrhistogram

面向 CP 的高动态范围（HDR）延迟直方图库，底层直连原生 C 引擎 `hdr_histogram.c`，提供 RAII 自动释放（`Histogram`）、协同遗漏（Coordinated Omission）校正、分位数查询与直方图合并能力。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| **上游源平台** | [crates.io/crates/hdrhistogram](https://crates.io/crates/hdrhistogram) |
| **上游原包名** | `hdrhistogram`（CP 版本 `1.0.0`） |
| **上游仓库** | https://github.com/HdrHistogram/HdrHistogram_rust |
| **许可证** | BSD-2-Clause（上游 Rust 封装：MIT） |

从 Rust 迁移到 CP 保持原包名不变：

```rust
// Rust
use hdrhistogram::Histogram;
let mut hist = Histogram::<u64>::new_with_bounds(1, 60_000_000, 3)?;
hist.record(42)?;
let p99 = hist.value_at_quantile(0.99);
```

```cp
// CP —— 保持原包名，原生 C 性能 + 自动 RAII 资源释放
import { Histogram } from "hdrhistogram";
```

## 安装

```bash
cap add hdrhistogram
```

## 实战示例

### 1. 记录延迟与分位数/百分位数查询

```cp
import { Histogram } from "hdrhistogram";

fn measureLatency(): !void {
    // 追踪 1us ~ 60_000_000us (60s)，保留 3 位有效数字精度
    mut h = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    h.init(1, 60_000_000, 3)?; // 离开作用域时通过 __cp_deinit 自动释放

    h.record(120)?;
    h.recordCount(250, 10)?;   // 记录 10 次 250us
    h.recordN(500, 5)?;        // recordCount 别名
    h.saturatingRecord(99_000_000)?; // 超出上界时自动饱和钳位而非报错

    let total = h.len()?;      // 17
    let empty = h.isEmpty()?;  // false
    let min_v = h.min()?;
    let max_v = h.max()?;
    let avg = h.mean()?;
    let sd = h.stddev()?;      // 别名：h.stdev()

    // 百分位（0.0..100.0）与分位数（0.0..1.0）查询
    let p99 = h.valueAtPercentile(99.0)?; // 别名：h.percentile(99.0)
    let q999 = h.valueAtQuantile(0.999)?;

    // 反向查询：小于等于给定阈值的样本占比
    let pct_under_300 = h.percentileBelow(300)?;
    let q_under_300 = h.quantileBelow(300)?;
}
```

### 2. 协同遗漏校正与多直方图合并

```cp
import { Histogram } from "hdrhistogram";

fn mergeWorkerHistograms(): !void {
    mut h1 = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    mut h2 = Histogram { handle: null, lowest_trackable: 0, highest_trackable: 0, sigfigs: 0 };
    h1.open(1, 10_000_000, 3)?;
    h2.open(1, 10_000_000, 3)?;

    // 当停顿超过 expected_interval (1000us) 时自动回填遗漏样本
    h1.recordCorrecting(5_000, 1_000)?;
    h2.recordCorrectingN(8_000, 2, 1_000)?;

    // 合并 h2 到 h1（返回越界丢弃样本数）
    let dropped = h1.add(h2)?;
    let mem_bytes = h1.memorySize()?;

    // 桶等价区间辅助查询
    let eq = h1.equivalent(1000, 1001)?;
    let lo_eq = h1.lowestEquivalent(5_000)?;
    let med_eq = h1.medianEquivalent(5_000)?;
    let next_ne = h1.nextNonEquivalent(5_000)?;
    let bucket_span = h1.sizeOfEquivalentRange(5_000)?;

    h1.reset()?;
}
```

## 性能与安全亮点

- **原生 C 引擎（`-O3`）**：直连工业级 `hdr_histogram.c` 并附带预编译静态库，单次记录达纳秒级。
- **确定性 RAII 释放**：`Histogram.__cp_deinit` 在作用域退出或错误早退时自动调用 `hdr_close`，杜绝 C 堆内存泄漏。
- **强类型错误联合（`HdrError`）**：明确区分 `InitFailed`、`InvalidHandle`、`ValueOutOfRange`；提供 `saturatingRecord` 安全处理异常离群值。
- **完整协同遗漏校正**：支持 `recordCorrected`/`recordCorrecting`/`recordCorrectingN` 与 `addCorrecting`，真实还原长尾延迟分布。

## API 参考

完整类型定义（含底层 C FFI 绑定）见 [`src/index.d.cp`](src/index.d.cp)，AI 智能体极简速查见 [`README_AGENT.md`](README_AGENT.md)。
