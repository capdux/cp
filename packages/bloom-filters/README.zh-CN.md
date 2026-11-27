[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bloom-filters

Zero-heap classic Bloom filter in pure CP with Kirsch-Mitzenmacher double hashing

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:bloom-filters` ([https://github.com/Callidon/bloom-filters](https://github.com/Callidon/bloom-filters)) / `crates.io:bloomfilter` ([https://github.com/jedisct1/rust-bloom-filter](https://github.com/jedisct1/rust-bloom-filter))（原库名 `bloom-filters` → CP 包名 `bloom-filters`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `bloom-filters` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bloom-filters
```

```cp
import {
    BloomFilter,
    fnv1a64,
    djb2Mix64,
    popcount,
    fillRatio,
    optimalNumHashes,
} from "bloom-filters";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BloomFilter` | `class` | 字段: `num_hashes: u32, bit_count: usize, inserted: u64` · 方法: `new, clear, insertBytes, insert, containsBytes, contains, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `fnv1a64` | `(data: &[]u8): u64` | 调用 `fnv1a64` 并返回 `u64`。 |
| `djb2Mix64` | `(data: &[]u8): u64` | 调用 `djb2Mix64` 并返回 `u64`。 |
| `popcount` | `(bits: &[]u8): u64` | 调用 `popcount` 并返回 `u64`。 |
| `fillRatio` | `(bits: &[]u8): f64` | 调用 `fillRatio` 并返回 `f64`。 |
| `optimalNumHashes` | `(bit_count: usize, expected_items: usize): u32` | 调用 `optimalNumHashes` 并返回 `u32`。 |

## 最小调用示例

```cp
import { fnv1a64 } from "bloom-filters";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = fnv1a64(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
