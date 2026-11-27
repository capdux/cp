[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rand_distr

Zero-heap probability distribution samplers (Uniform, Normal Box-Muller, Exponential, Bernoulli, WeightedIndex) built on xoshiro256 (clean-room port of Rust rand_distr)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rand_distr` ([https://github.com/rust-random/rand](https://github.com/rust-random/rand))（原库名 `rand_distr` → CP 包名 `rand_distr`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `rand_distr` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rand_distr
```

```cp
import {
    UniformRange,
    NormalDistr,
    ExpDistr,
    BernoulliDistr,
    WeightedIndex,
    rdstAbs,
    rdstSqrt,
    rdstLn,
} from "rand_distr";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `UniformRange` | `class` | 字段: `low: f64, high: f64` · 方法: `new, sample` |
| `NormalDistr` | `class` | 字段: `mean: f64, std_dev: f64` · 方法: `new, sample` |
| `ExpDistr` | `class` | 字段: `lambda: f64` · 方法: `new, sample` |
| `BernoulliDistr` | `class` | 字段: `p: f64` · 方法: `new, sample` |
| `WeightedIndex` | `class` | 字段: `cumulative: [16]f64, count: usize, total: f64` · 方法: `fromWeights, sample` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rdstAbs` | `(x: f64): f64` | 调用 `rdstAbs` 并返回 `f64`。 |
| `rdstSqrt` | `(x: f64): f64` | 调用 `rdstSqrt` 并返回 `f64`。 |
| `rdstLn` | `(x: f64): f64` | 调用 `rdstLn` 并返回 `f64`。 |
| `rdstCos` | `(angle: f64): f64` | 调用 `rdstCos` 并返回 `f64`。 |
| `rdstOpen01` | `(rng: &mut Xoshiro256): f64` | 调用 `rdstOpen01` 并返回 `f64`。 |

## 最小调用示例

```cp
import { rdstAbs } from "rand_distr";

pub fn main() {
    let result = rdstAbs(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
