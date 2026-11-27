[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hamming

Zero-allocation Hamming distance, byte popcount weight, 64-bit Charikar SimHash, and Hamming(7,4) error-correcting code in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hamming` ([https://github.com/huonw/hamming](https://github.com/huonw/hamming))

| 元信息 | 值 |
| --- | --- |
| 包名 | `hamming` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add hamming
```

```cp
import {
    Hamming74Decoded,
    popcountU32,
    popcountU8,
    popcountU64,
    hammingU64,
    weightBytes,
    distanceBytes,
    isTokenByte,
} from "hamming";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Hamming74Decoded` | `class` | 字段: `data: u8, corrected: bool, error_bit: u8` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `popcountU32` | `(x: u32): u32` | 调用 `popcountU32` 并返回 `u32`。 |
| `popcountU8` | `(b: u8): u32` | 调用 `popcountU8` 并返回 `u32`。 |
| `popcountU64` | `(x: u64): u32` | 调用 `popcountU64` 并返回 `u32`。 |
| `hammingU64` | `(a: u64, b: u64): u32` | 调用 `hammingU64` 并返回 `u32`。 |
| `weightBytes` | `(data: &[]u8): u64` | 调用 `weightBytes` 并返回 `u64`。 |
| `distanceBytes` | `(a: &[]u8, b: &[]u8): i64` | 调用 `distanceBytes` 并返回 `i64`。 |
| `isTokenByte` | `(b: u8): bool` | 执行 `isTokenByte` 条件判定，返回 `bool`。 |
| `hashToken64` | `(text: &[]u8, start: usize, end: usize): u64` | 执行 `hashToken64` 条件判定，返回 `bool`。 |
| `simhash64` | `(text: &[]u8): u64` | 调用 `simhash64` 并返回 `u64`。 |
| `hamming74Encode` | `(nibble: u8): u8` | 调用 `hamming74Encode` 并返回 `u8`。 |
| `hamming74Decode` | `(codeword: u8): Hamming74Decoded` | 调用 `hamming74Decode` 并返回 `Hamming74Decoded`。 |

## 最小调用示例

```cp
import { isTokenByte } from "hamming";

pub fn main() {
    let result = isTokenByte(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
