[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# lerp

Zero-heap interpolation, range remapping, angle conversion, and easing curves in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:lerp` ([https://github.com/mattdesl/lerp](https://github.com/mattdesl/lerp)) / `npm:eases` ([https://github.com/mattdesl/eases](https://github.com/mattdesl/eases)) / `npm:smoothstep` ([https://github.com/mikolalysenko/smoothstep](https://github.com/mikolalysenko/smoothstep)) / `crates.io:interpolation` ([https://github.com/PistonDevelopers/interpolation](https://github.com/PistonDevelopers/interpolation)) / `crates.io:keyframe` ([https://github.com/HannesMann/keyframe](https://github.com/HannesMann/keyframe))（原库名 `lerp` → CP 包名 `lerp`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `lerp` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add lerp
```

```cp
import {
    absF64,
    minF64,
    maxF64,
    clamp,
    clamp01,
    approxEq,
    lerp,
    lerpClamped,
} from "lerp";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `absF64` | `(x: f64): f64` | 调用 `absF64` 并返回 `f64`。 |
| `minF64` | `(a: f64, b: f64): f64` | 调用 `minF64` 并返回 `f64`。 |
| `maxF64` | `(a: f64, b: f64): f64` | 调用 `maxF64` 并返回 `f64`。 |
| `clamp` | `(x: f64, lo: f64, hi: f64): f64` | 调用 `clamp` 并返回 `f64`。 |
| `clamp01` | `(x: f64): f64` | 调用 `clamp01` 并返回 `f64`。 |
| `approxEq` | `(a: f64, b: f64, eps: f64): bool` | 调用 `approxEq` 并返回 `bool`。 |
| `lerp` | `(a: f64, b: f64, t: f64): f64` | 调用 `lerp` 并返回 `f64`。 |
| `lerpClamped` | `(a: f64, b: f64, t: f64): f64` | 调用 `lerpClamped` 并返回 `f64`。 |
| `inverseLerp` | `(a: f64, b: f64, value: f64): f64` | 调用 `inverseLerp` 并返回 `f64`。 |
| `remap` | `(in_min: f64, in_max: f64, out_min: f64, out_max: f64, value: f64): f64` | 调用 `remap` 并返回 `f64`。 |
| `remapClamped` | `(in_min: f64, in_max: f64, out_min: f64, out_max: f64, value: f64): f64` | 调用 `remapClamped` 并返回 `f64`。 |
| `smoothstep` | `(edge0: f64, edge1: f64, x: f64): f64` | 调用 `smoothstep` 并返回 `f64`。 |
| `smootherstep` | `(edge0: f64, edge1: f64, x: f64): f64` | 调用 `smootherstep` 并返回 `f64`。 |
| `step` | `(edge: f64, x: f64): f64` | 调用 `step` 并返回 `f64`。 |
| `wrap` | `(x: f64, lo: f64, hi: f64): f64` | 调用 `wrap` 并返回 `f64`。 |
| `pingPong` | `(t: f64, length: f64): f64` | 调用 `pingPong` 并返回 `f64`。 |
| `degToRad` | `(deg: f64): f64` | 调用 `degToRad` 并返回 `f64`。 |
| `radToDeg` | `(rad: f64): f64` | 调用 `radToDeg` 并返回 `f64`。 |
| `easeInQuad` | `(t: f64): f64` | 调用 `easeInQuad` 并返回 `f64`。 |
| `easeOutQuad` | `(t: f64): f64` | 调用 `easeOutQuad` 并返回 `f64`。 |
| `easeInOutQuad` | `(t: f64): f64` | 调用 `easeInOutQuad` 并返回 `f64`。 |
| `easeInCubic` | `(t: f64): f64` | 调用 `easeInCubic` 并返回 `f64`。 |
| `easeOutCubic` | `(t: f64): f64` | 调用 `easeOutCubic` 并返回 `f64`。 |
| `easeInOutCubic` | `(t: f64): f64` | 调用 `easeInOutCubic` 并返回 `f64`。 |

## 最小调用示例

```cp
import { absF64 } from "lerp";

pub fn main() {
    let result = absF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
