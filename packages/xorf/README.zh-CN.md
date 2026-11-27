[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# xorf

Zero-allocation Xor8 static fingerprint filter (Graf & Lemire ACM JEA 2020 / Rust xorf parity) with O(1) 3-lookup XOR queries and ~0.39% false-positive rate for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:xorf` ([https://github.com/ayazhafiz/xorf](https://github.com/ayazhafiz/xorf))（原库名 `xorf` → CP 包名 `xorf`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `xorf` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add xorf
```

```cp
import {
    Slots3,
    Xor8Filter,
    makeU64,
    hashKey64,
    deriveSlots,
} from "xorf";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Slots3` | `class` | 字段: `h0: usize, h1: usize, h2: usize, fp: u8` |
| `Xor8Filter` | `class` | 字段: `seed: u64, block_length: usize, fingerprints: [MAX_SLOTS]u8, key_count: usize, valid: bool` · 方法: `emptyWithStatus, build, contains, len, fingerprintBytes, isValid` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | 调用 `makeU64` 并返回 `u64`。 |
| `hashKey64` | `(key: u64, seed: u64): u64` | 执行 `hashKey64` 条件判定，返回 `bool`。 |
| `deriveSlots` | `(h: u64, block_len: usize): Slots3` | 调用 `deriveSlots` 并返回 `Slots3`。 |

## 最小调用示例

```cp
import { hashKey64 } from "xorf";

pub fn main() {
    let result = hashKey64(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
