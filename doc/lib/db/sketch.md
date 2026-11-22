# CP 标准库：流式概率素描算法套件手册 (`cap:sketch`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:db`，导入请使用 `import { ... } from "cap:db"`。

## 一、 架构总览与设计准则

`cap:sketch` 是 CP 语言官方内置的纯 CP **流式概率素描算法套件**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，为大规模遥测监控、实时 UV 去重统计、流式延迟分位数观测与无偏流式抽样提供恒定有界内存的高精度实现。既可通过 `import { ... } from "cap:db"` 导入，也支持 `import cap from "cap"; cap.sketch.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/sketch/sketch.cp` | `// Source: cp/lib/sketch/sketch.cp (...)` | `SketchError`、`HyperLogLog`、`Centroid`、`QuantileDigest`、`ReservoirSampler(T)` |
| `lib/sketch/index.cp` | `// Source: cp/lib/sketch/index.cp (...)` | `cap:sketch` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `SketchError`

- `InvalidPrecision`：`HyperLogLog.init` 传入超出 `[4, 16]` 范围的精度 `precision`。
- `InvalidQuantile`：`QuantileDigest.quantile` 传入 `NaN` 或超出 `[0.0, 1.0]` 的分位点 `q`。
- `InvalidCapacity`：`QuantileDigest.init` 传入 `< 2` 的质心上限，或 `ReservoirSampler.init` 传入 `0` 容量。
- `IncompatibleMerge`：`HyperLogLog.merge` 合并两个不同 `precision` 的实例。
- `InvalidValue`：`QuantileDigest.add` 传入 `NaN` 或 `+/-Inf`。
- `OutOfMemory`：底层桶数组、质心缓冲区或蓄水池切片内存分配失败。

### 2. `HyperLogLog`（恒定 $2^p$ 字节内存基数估算）

- **初始化与生命周期**：
  - `HyperLogLog.init(alloc, precision: u8): HyperLogLog ! SketchError`（`precision` 范围 `[4, 16]`，如 `12` 占 4 KiB、`14` 占 16 KiB）
  - `deinit(*_)` / `reset(*_)`
  - `getPrecision(): u8` / `bucketCount(): usize`
- **观测与合并**：
  - `add(*_, item: []const u8)`（FNV-1a + SplitMix64 终结混洗）
  - `addU64(*_, v: u64)`（SplitMix64 64-bit 终结混洗）
  - `count(): u64`（精确 $\alpha_m m^2 / \sum 2^{-M[j]}$ 调和平均 + 小范围 LinearCounting 修正）
  - `merge(*_, other: HyperLogLog): void ! SketchError`（校验同 `precision`，逐桶取 `max`）

### 3. `QuantileDigest`（流式有界质心分位数汇总器）

- **初始化与生命周期**：
  - `QuantileDigest.init(alloc, max_centroids: usize): QuantileDigest ! SketchError`（`max_centroids >= 2`）
  - `deinit(*_)` / `reset(*_)`
- **流式观测与查询**：
  - `add(*_, val: f64): void ! SketchError`（拒绝 `NaN`/`Inf`，缓冲区满时自动排序并按局部密度 $4nq(1-q)/\delta$ 合并相邻质心，严格保持质心数 $\le \text{max\_centroids}$）
  - `quantile(*_, q: f64): f64? ! SketchError`（校验 `0.0 <= q <= 1.0`，按质心中点线性插值估算；空摘要返回 `null`）
  - `min(): f64?` / `max(): f64?` / `count(): u64`
  - `merge(*_, other: QuantileDigest)`（合并远端质心并压缩）

### 4. `ReservoirSampler(comptime T: type)`（Algorithm R 等概率蓄水池抽样）

- **初始化与生命周期**：
  - `ReservoirSampler(T).init(alloc, capacity: usize, seed: u64): Self ! SketchError`（`capacity >= 1`）
  - `deinit(*_)` / `reset(*_, seed: u64)`
- **流式采样与读取**：
  - `observe(*_, item: T)`（在未知总长 $N$ 的数据流中以精确 $k/N$ 概率保留样本）
  - `samples(): []const T`
  - `seenCount(): u64`

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { HyperLogLog, QuantileDigest, ReservoirSampler } from "cap:db";
import cap from "cap";

pub fn demoSketch(): ! {
    const alloc = std.heap.page_allocator;

    let hll = HyperLogLog.init(alloc, 12)?;
    defer hll.deinit();
    hll.add("user:1001");
    hll.addU64(42);
    const uv = hll.count();
    * = uv;

    let qd = cap.sketch.QuantileDigest.init(alloc, 128)?;
    defer qd.deinit();
    qd.add(12.5)?;
    qd.add(48.0)?;
    const p95 = qd.quantile(0.95)?;
    * = p95;

    let sampler = ReservoirSampler(u64).init(alloc, 16, 12345)?;
    defer sampler.deinit();
    sampler.observe(99);
}
```
