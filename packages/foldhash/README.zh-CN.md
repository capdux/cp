[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# foldhash

Zero-allocation 64x64->128-bit folded-multiply non-cryptographic hasher with Fast and Quality modes (Rust foldhash parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:foldhash` ([https://github.com/orlp/foldhash](https://github.com/orlp/foldhash))

| 元信息 | 值 |
| --- | --- |
| 包名 | `foldhash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add foldhash
```

```cp
import {
    FoldHasher,
    makeU64,
    addWrap64,
    foldedMultiply,
    byteShift64,
    initFoldSeed,
    initAccum,
    hashBytesFast,
} from "foldhash";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FoldHasher` | `class` | 字段: `accum: u64, sponge: u64, sponge_len: u8, fold_seed: u64, quality: bool, total_len: u64` · 方法: `initFast, initQuality, writeU8, writeU32, writeU64, writeBytes, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | 调用 `makeU64` 并返回 `u64`。 |
| `addWrap64` | `(a: u64, b: u64): u64` | 写入或更新状态数据。 |
| `foldedMultiply` | `(x: u64, y: u64): u64` | 调用 `foldedMultiply` 并返回 `u64`。 |
| `byteShift64` | `(idx: u8): u64` | 调用 `byteShift64` 并返回 `u64`。 |
| `initFoldSeed` | `(seed: u64): u64` | 创建或初始化实例，返回 `u64`。 |
| `initAccum` | `(seed: u64): u64` | 创建或初始化实例，返回 `u64`。 |
| `hashBytesFast` | `(data: &[]u8, seed: u64): u64` | 执行 `hashBytesFast` 条件判定，返回 `bool`。 |
| `hashBytesQuality` | `(data: &[]u8, seed: u64): u64` | 执行 `hashBytesQuality` 条件判定，返回 `bool`。 |
| `hashU64` | `(v: u64, seed: u64): u64` | 执行 `hashU64` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { hashBytesFast } from "foldhash";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = hashBytesFast(out_buf[0..256], 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
