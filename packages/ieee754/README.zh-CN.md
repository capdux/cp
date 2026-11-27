[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ieee754

Zero-allocation IEEE-754 32-bit and 64-bit floating-point binary encoder/decoder (little-endian & big-endian) with NaN, Infinity, and subnormal fidelity (clean-room MIT implementation of npm ieee754, #293 / 173M wk)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ieee754` ([https://github.com/feross/ieee754](https://github.com/feross/ieee754)) | 许可：`BSD-3-Clause`

| 元信息 | 值 |
| --- | --- |
| 包名 | `ieee754` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ieee754
```

```cp
import {
    readF32Le,
    readF32Be,
    writeF32Le,
    writeF32Be,
    readF64Le,
    readF64Be,
    writeF64Le,
    writeF64Be,
} from "ieee754";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `readF32Le` | `(buf: &[]u8, offset: usize): f32` | 调用 `readF32Le` 并返回 `f32`。 |
| `readF32Be` | `(buf: &[]u8, offset: usize): f32` | 调用 `readF32Be` 并返回 `f32`。 |
| `writeF32Le` | `(buf: []u8, offset: usize, value: f32): bool` | 调用 `writeF32Le` 并返回 `bool`。 |
| `writeF32Be` | `(buf: []u8, offset: usize, value: f32): bool` | 调用 `writeF32Be` 并返回 `bool`。 |
| `readF64Le` | `(buf: &[]u8, offset: usize): f64` | 调用 `readF64Le` 并返回 `f64`。 |
| `readF64Be` | `(buf: &[]u8, offset: usize): f64` | 调用 `readF64Be` 并返回 `f64`。 |
| `writeF64Le` | `(buf: []u8, offset: usize, value: f64): bool` | 调用 `writeF64Le` 并返回 `bool`。 |
| `writeF64Be` | `(buf: []u8, offset: usize, value: f64): bool` | 调用 `writeF64Be` 并返回 `bool`。 |
| `readIeee754` | `(buf: &[]u8, offset: usize, is_le: bool, n_bytes: usize): f64` | 调用 `readIeee754` 并返回 `f64`。 |
| `writeIeee754` | `(buf: []u8, value: f64, offset: usize, is_le: bool, n_bytes: usize): bool` | 调用 `writeIeee754` 并返回 `bool`。 |

## 最小调用示例

```cp
import { readF32Le } from "ieee754";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = readF32Le(out_buf[0..256], 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
