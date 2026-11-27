[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ezing

Zero-allocation Robert Penner animation easing functions and CSS cubic-bezier solver (Rust ezing / keyframe / npm eases parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ezing` ([https://github.com/NyxCode/ezing](https://github.com/NyxCode/ezing))（原库名 `ezing` → CP 包名 `ezing`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `ezing` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ezing
```

```cp
import {
    clamp01,
    absF64,
    sqrtF64,
    sinF64,
    cosF64,
    linear,
    quadIn,
    quadOut,
} from "ezing";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `clamp01` | `(t: f64): f64` | 调用 `clamp01` 并返回 `f64`。 |
| `absF64` | `(x: f64): f64` | 调用 `absF64` 并返回 `f64`。 |
| `sqrtF64` | `(x: f64): f64` | 调用 `sqrtF64` 并返回 `f64`。 |
| `sinF64` | `(x_in: f64): f64` | 调用 `sinF64` 并返回 `f64`。 |
| `cosF64` | `(x: f64): f64` | 调用 `cosF64` 并返回 `f64`。 |
| `linear` | `(t: f64): f64` | 调用 `linear` 并返回 `f64`。 |
| `quadIn` | `(t_in: f64): f64` | 调用 `quadIn` 并返回 `f64`。 |
| `quadOut` | `(t_in: f64): f64` | 调用 `quadOut` 并返回 `f64`。 |
| `quadInOut` | `(t_in: f64): f64` | 调用 `quadInOut` 并返回 `f64`。 |
| `cubicIn` | `(t_in: f64): f64` | 调用 `cubicIn` 并返回 `f64`。 |
| `cubicOut` | `(t_in: f64): f64` | 调用 `cubicOut` 并返回 `f64`。 |
| `cubicInOut` | `(t_in: f64): f64` | 调用 `cubicInOut` 并返回 `f64`。 |
| `quartIn` | `(t_in: f64): f64` | 调用 `quartIn` 并返回 `f64`。 |
| `quartOut` | `(t_in: f64): f64` | 调用 `quartOut` 并返回 `f64`。 |
| `quartInOut` | `(t_in: f64): f64` | 调用 `quartInOut` 并返回 `f64`。 |
| `quintIn` | `(t_in: f64): f64` | 调用 `quintIn` 并返回 `f64`。 |
| `quintOut` | `(t_in: f64): f64` | 调用 `quintOut` 并返回 `f64`。 |
| `quintInOut` | `(t_in: f64): f64` | 调用 `quintInOut` 并返回 `f64`。 |
| `sineIn` | `(t_in: f64): f64` | 调用 `sineIn` 并返回 `f64`。 |
| `sineOut` | `(t_in: f64): f64` | 调用 `sineOut` 并返回 `f64`。 |
| `sineInOut` | `(t_in: f64): f64` | 调用 `sineInOut` 并返回 `f64`。 |
| `circIn` | `(t_in: f64): f64` | 调用 `circIn` 并返回 `f64`。 |
| `circOut` | `(t_in: f64): f64` | 调用 `circOut` 并返回 `f64`。 |
| `circInOut` | `(t_in: f64): f64` | 调用 `circInOut` 并返回 `f64`。 |
| `backIn` | `(t_in: f64): f64` | 调用 `backIn` 并返回 `f64`。 |
| `backOut` | `(t_in: f64): f64` | 调用 `backOut` 并返回 `f64`。 |
| `backInOut` | `(t_in: f64): f64` | 调用 `backInOut` 并返回 `f64`。 |
| `bounceOut` | `(t_in: f64): f64` | 调用 `bounceOut` 并返回 `f64`。 |
| `bounceIn` | `(t_in: f64): f64` | 调用 `bounceIn` 并返回 `f64`。 |
| `bounceInOut` | `(t_in: f64): f64` | 调用 `bounceInOut` 并返回 `f64`。 |
| `sampleCurve` | `(a: f64, b: f64, c: f64, u: f64): f64` | 调用 `sampleCurve` 并返回 `f64`。 |
| `sampleCurveDeriv` | `(a: f64, b: f64, c: f64, u: f64): f64` | 调用 `sampleCurveDeriv` 并返回 `f64`。 |
| `cubicBezier` | `(x1: f64, y1: f64, x2: f64, y2: f64, x_in: f64): f64` | 调用 `cubicBezier` 并返回 `f64`。 |

## 最小调用示例

```cp
import { clamp01 } from "ezing";

pub fn main() {
    let result = clamp01(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
