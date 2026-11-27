[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fnv

Zero-allocation FNV-1a 32/64-bit and Rust rustc-hash FxHash 64-bit streaming non-cryptographic hashers for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustc-hash` ([https://github.com/rust-lang/rustc-hash](https://github.com/rust-lang/rustc-hash)) / `crates.io:fnv` ([https://github.com/servo/rust-fnv](https://github.com/servo/rust-fnv))（原库名 `rustc-hash` → CP 包名 `fnv`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `fnv` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fnv
```

```cp
import {
    Fnv32Hasher,
    Fnv64Hasher,
    FxHasher,
    mulWrap32,
    mulWrap64,
    rotl64By5,
    fnv1a32,
    fnv1a64,
} from "fnv";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Fnv32Hasher` | `class` | 字段: `state: u32` · 方法: `init, writeByte, write, finish` |
| `Fnv64Hasher` | `class` | 字段: `state: u64` · 方法: `init, writeByte, write, finish` |
| `FxHasher` | `class` | 字段: `state: u64` · 方法: `init, withSeed, writeU64, writeBytes, finish` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mulWrap32` | `(a: u32, b: u32): u32` | 调用 `mulWrap32` 并返回 `u32`。 |
| `mulWrap64` | `(a: u64, b: u64): u64` | 调用 `mulWrap64` 并返回 `u64`。 |
| `rotl64By5` | `(x: u64): u64` | 调用 `rotl64By5` 并返回 `u64`。 |
| `fnv1a32` | `(data: string): u32` | 调用 `fnv1a32` 并返回 `u32`。 |
| `fnv1a64` | `(data: string): u64` | 调用 `fnv1a64` 并返回 `u64`。 |
| `fxHash64` | `(data: string): u64` | 调用 `fxHash64` 并返回 `u64`。 |

## 最小调用示例

```cp
import { fnv1a32 } from "fnv";

pub fn main() {
    let result = fnv1a32("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
