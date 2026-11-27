[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# d3-ease

Zero-allocation D3 easing functions (linear, quad, cubic, poly, sin, exp, circle, bounce, back, elastic) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:d3-ease` ([https://github.com/d3/d3-ease](https://github.com/d3/d3-ease))

| 元信息 | 值 |
| --- | --- |
| 包名 | `d3-ease` |
| 版本 | `1.0.0` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add d3-ease
```

```cp
import {
    deasTpmt,
    deasClampAmp,
    easeLinear,
    easeQuadIn,
    easeQuadOut,
    easeQuadInOut,
    easeCubicIn,
    easeCubicOut,
} from "d3-ease";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `deasTpmt` | `(x: f64): f64` | 调用 `deasTpmt` 并返回 `f64`。 |
| `deasClampAmp` | `(a: f64): f64` | 调用 `deasClampAmp` 并返回 `f64`。 |
| `easeLinear` | `(t: f64): f64` | 调用 `easeLinear` 并返回 `f64`。 |
| `easeQuadIn` | `(t: f64): f64` | 调用 `easeQuadIn` 并返回 `f64`。 |
| `easeQuadOut` | `(t: f64): f64` | 调用 `easeQuadOut` 并返回 `f64`。 |
| `easeQuadInOut` | `(t: f64): f64` | 调用 `easeQuadInOut` 并返回 `f64`。 |
| `easeCubicIn` | `(t: f64): f64` | 调用 `easeCubicIn` 并返回 `f64`。 |
| `easeCubicOut` | `(t: f64): f64` | 调用 `easeCubicOut` 并返回 `f64`。 |
| `easeCubicInOut` | `(t: f64): f64` | 调用 `easeCubicInOut` 并返回 `f64`。 |
| `easePolyInExp` | `(t: f64, e: f64): f64` | 调用 `easePolyInExp` 并返回 `f64`。 |
| `easePolyOutExp` | `(t: f64, e: f64): f64` | 调用 `easePolyOutExp` 并返回 `f64`。 |
| `easePolyInOutExp` | `(t: f64, e: f64): f64` | 调用 `easePolyInOutExp` 并返回 `f64`。 |
| `easePolyIn` | `(t: f64): f64` | 调用 `easePolyIn` 并返回 `f64`。 |
| `easePolyOut` | `(t: f64): f64` | 调用 `easePolyOut` 并返回 `f64`。 |
| `easePolyInOut` | `(t: f64): f64` | 调用 `easePolyInOut` 并返回 `f64`。 |
| `easeSinIn` | `(t: f64): f64` | 调用 `easeSinIn` 并返回 `f64`。 |
| `easeSinOut` | `(t: f64): f64` | 调用 `easeSinOut` 并返回 `f64`。 |
| `easeSinInOut` | `(t: f64): f64` | 调用 `easeSinInOut` 并返回 `f64`。 |
| `easeExpIn` | `(t: f64): f64` | 调用 `easeExpIn` 并返回 `f64`。 |
| `easeExpOut` | `(t: f64): f64` | 调用 `easeExpOut` 并返回 `f64`。 |
| `easeExpInOut` | `(t: f64): f64` | 调用 `easeExpInOut` 并返回 `f64`。 |
| `easeCircleIn` | `(t: f64): f64` | 调用 `easeCircleIn` 并返回 `f64`。 |
| `easeCircleOut` | `(t: f64): f64` | 调用 `easeCircleOut` 并返回 `f64`。 |
| `easeCircleInOut` | `(t: f64): f64` | 调用 `easeCircleInOut` 并返回 `f64`。 |
| `easeBounceOut` | `(t: f64): f64` | 调用 `easeBounceOut` 并返回 `f64`。 |
| `easeBounceIn` | `(t: f64): f64` | 调用 `easeBounceIn` 并返回 `f64`。 |
| `easeBounceInOut` | `(t: f64): f64` | 调用 `easeBounceInOut` 并返回 `f64`。 |
| `easeBackInOvershoot` | `(t: f64, s: f64): f64` | 调用 `easeBackInOvershoot` 并返回 `f64`。 |
| `easeBackOutOvershoot` | `(t: f64, s: f64): f64` | 调用 `easeBackOutOvershoot` 并返回 `f64`。 |
| `easeBackInOutOvershoot` | `(t: f64, s: f64): f64` | 调用 `easeBackInOutOvershoot` 并返回 `f64`。 |
| `easeBackIn` | `(t: f64): f64` | 调用 `easeBackIn` 并返回 `f64`。 |
| `easeBackOut` | `(t: f64): f64` | 调用 `easeBackOut` 并返回 `f64`。 |
| `easeBackInOut` | `(t: f64): f64` | 调用 `easeBackInOut` 并返回 `f64`。 |
| `easeElasticInParams` | `(t: f64, a: f64, p: f64): f64` | 调用 `easeElasticInParams` 并返回 `f64`。 |
| `easeElasticOutParams` | `(t: f64, a: f64, p: f64): f64` | 调用 `easeElasticOutParams` 并返回 `f64`。 |
| `easeElasticInOutParams` | `(t: f64, a: f64, p: f64): f64` | 调用 `easeElasticInOutParams` 并返回 `f64`。 |
| `easeElasticIn` | `(t: f64): f64` | 调用 `easeElasticIn` 并返回 `f64`。 |
| `easeElasticOut` | `(t: f64): f64` | 调用 `easeElasticOut` 并返回 `f64`。 |
| `easeElasticInOut` | `(t: f64): f64` | 调用 `easeElasticInOut` 并返回 `f64`。 |

## 最小调用示例

```cp
import { deasTpmt } from "d3-ease";

pub fn main() {
    let result = deasTpmt(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
