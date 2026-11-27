[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# wyhash

Zero-allocation Wang Yi wyhash v4 64-bit hash and WyRng PRNG with overflow-free 128-bit multiply in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:wyhash` ([https://github.com/eldruin/wyhash-rs](https://github.com/eldruin/wyhash-rs))

| 元信息 | 值 |
| --- | --- |
| 包名 | `wyhash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add wyhash
```

```cp
import {
    Mul128,
    WyRng,
    makeU64,
    mul128,
    wymum,
    read4,
    read8,
    wyhashBytes,
} from "wyhash";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Mul128` | `class` | 字段: `lo: u64, hi: u64` |
| `WyRng` | `class` | 字段: `state: u64` · 方法: `new, nextU64, nextU32, nextF64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | 调用 `makeU64` 并返回 `u64`。 |
| `mul128` | `(a: u64, b: u64): Mul128` | 调用 `mul128` 并返回 `Mul128`。 |
| `wymum` | `(a: u64, b: u64): u64` | 调用 `wymum` 并返回 `u64`。 |
| `read4` | `(data: &[]u8, off: usize): u64` | 调用 `read4` 并返回 `u64`。 |
| `read8` | `(data: &[]u8, off: usize): u64` | 调用 `read8` 并返回 `u64`。 |
| `wyhashBytes` | `(data: &[]u8, seed: u64): u64` | 调用 `wyhashBytes` 并返回 `u64`。 |
| `wyhash` | `(data: string, seed: u64): u64` | 调用 `wyhash` 并返回 `u64`。 |

## 最小调用示例

```cp
import { wyhash } from "wyhash";

pub fn main() {
    let result = wyhash("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
