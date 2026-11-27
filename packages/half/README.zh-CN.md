[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# half

Zero-heap IEEE 754 binary16 (f16) and Google Brain Float 16 (bf16) codec and arithmetic in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:half` ([https://github.com/VoidStarKat/half-rs](https://github.com/VoidStarKat/half-rs)) / `npm:@petamoriken/float16` ([https://github.com/petamoriken/float16](https://github.com/petamoriken/float16))（原库名 `half` → CP 包名 `half`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `half` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add half
```

```cp
import {
    F16,
    Bf16,
    f32ToBits,
    bitsToF32,
    f32SliceToF16,
    f16SliceToF32,
} from "half";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `F16` | `class` | 字段: `bits: u16` · 方法: `fromBits, fromLeBytes, fromF32, toF32, isNan, isInfinite, +4 项` |
| `Bf16` | `class` | 字段: `bits: u16` · 方法: `fromBits, fromF32, toF32, isNan, isInfinite, neg, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `f32ToBits` | `(x: f32): u32` | 调用 `f32ToBits` 并返回 `u32`。 |
| `bitsToF32` | `(b: u32): f32` | 调用 `bitsToF32` 并返回 `f32`。 |
| `f32SliceToF16` | `(src: &[]f32, dst: []u16): usize` | 调用 `f32SliceToF16` 并返回 `usize`。 |
| `f16SliceToF32` | `(src: &[]u16, dst: []f32): usize` | 调用 `f16SliceToF32` 并返回 `usize`。 |

## 最小调用示例

```cp
import { f32ToBits } from "half";

pub fn main() {
    let result = f32ToBits(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
