[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# tdigest

Zero-allocation Merging T-Digest streaming quantile (P50/P95/P99/P99.9) and CDF estimator with distributed merge for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tdigest` ([https://github.com/MnO2/t-digest](https://github.com/MnO2/t-digest))

| 元信息 | 值 |
| --- | --- |
| 包名 | `tdigest` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add tdigest
```

```cp
import { Centroid, TDigest, emptyCentroid } from "tdigest";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Centroid` | `class` | 字段: `mean: f64, weight: f64` |
| `TDigest` | `class` | 字段: `centroids: [128]Centroid, centroid_len: usize, compression: f64, total_weight: f64, min_val: f64, max_val: f64, +1 项` · 方法: `init, count, min, max, centroidCount, isEmpty, +9 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `emptyCentroid` | `(): Centroid` | 调用 `emptyCentroid` 并返回 `Centroid`。 |

## 最小调用示例

```cp
import { emptyCentroid } from "tdigest";

pub fn main() {
    let result = emptyCentroid();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
