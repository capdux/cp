[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# kalman-filters

Zero-allocation 1D scalar and 2D constant-velocity Kalman filters with Joseph-form covariance update (Rust kalman-filters / kfilter parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:kalman-filters` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `kalman-filters` → CP 包名 `kalman-filters`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `kalman-filters` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add kalman-filters
```

```cp
import { Kalman1D, Kalman2D } from "kalman-filters";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Kalman1D` | `class` | 字段: `x: f64, p: f64, q: f64, r: f64, k: f64` · 方法: `init, predict, update, step, estimate, variance, +2 项` |
| `Kalman2D` | `class` | 字段: `pos: f64, vel: f64, p00: f64, p01: f64, p10: f64, p11: f64, +3 项` · 方法: `init, predict, updatePosition, position, velocity` |

## 最小调用示例

```cp
import { Kalman1D } from "kalman-filters";

pub fn main() {
    // Refer to src/index.d.cp for Kalman1D fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
