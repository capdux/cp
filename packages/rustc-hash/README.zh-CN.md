[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# rustc-hash

Zero-heap pure-CP fast non-cryptographic FxHasher (64-bit & 32-bit, crates.io rustc-hash equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustc-hash` ([https://github.com/rust-lang/rustc-hash](https://github.com/rust-lang/rustc-hash))

| 元信息 | 值 |
| --- | --- |
| 包名 | `rustc-hash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add rustc-hash
```

```cp
import {
    FxHasher64,
    FxHasher32,
    fxhMul64,
    fxhMul32,
    fxhRotl64By5,
    fxhRotl32By5,
    fxhAddWord64,
    fxhAddWord32,
} from "rustc-hash";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FxHasher64` | `class` | 字段: `hash: u64` · 方法: `new, withSeed, writeUsize, writeU64, writeU32, writeBytes, +3 项` |
| `FxHasher32` | `class` | 字段: `hash: u32` · 方法: `new, withSeed, writeU32, writeBytes, writeStr, finish, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `fxhMul64` | `(a: u64, b: u64): u64` | 调用 `fxhMul64` 并返回 `u64`。 |
| `fxhMul32` | `(a: u32, b: u32): u32` | 调用 `fxhMul32` 并返回 `u32`。 |
| `fxhRotl64By5` | `(x: u64): u64` | 调用 `fxhRotl64By5` 并返回 `u64`。 |
| `fxhRotl32By5` | `(x: u32): u32` | 调用 `fxhRotl32By5` 并返回 `u32`。 |
| `fxhAddWord64` | `(hash: u64, word: u64): u64` | 调用 `fxhAddWord64` 并返回 `u64`。 |
| `fxhAddWord32` | `(hash: u32, word: u32): u32` | 调用 `fxhAddWord32` 并返回 `u32`。 |
| `fxhHashSlice64` | `(hash: u64, data: &[]u8): u64` | 调用 `fxhHashSlice64` 并返回 `u64`。 |
| `fxhHashSlice32` | `(hash: u32, data: &[]u8): u32` | 调用 `fxhHashSlice32` 并返回 `u32`。 |
| `fxHash64` | `(data: []u8): u64` | 调用 `fxHash64` 并返回 `u64`。 |
| `fxHash64Str` | `(s: string): u64` | 调用 `fxHash64Str` 并返回 `u64`。 |
| `fxHash32` | `(data: []u8): u32` | 调用 `fxHash32` 并返回 `u32`。 |

## 最小调用示例

```cp
import { fxHash64Str } from "rustc-hash";

pub fn main() {
    let result = fxHash64Str("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
