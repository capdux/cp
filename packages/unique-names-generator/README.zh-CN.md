[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unique-names-generator

Zero-allocation deterministic human-readable name generator for containers, sandboxes & subdomains (unique-names-generator / human-id / Rust petname parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:unique-names-generator` ([https://github.com/andreasonny83/unique-names-generator](https://github.com/andreasonny83/unique-names-generator))（原库名 `unique-names-generator` → CP 包名 `unique-names-generator`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `unique-names-generator` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add unique-names-generator
```

```cp
import {
    Petname,
    splitmix64Next,
    adjectiveAt,
    adverbAt,
    nounAt,
    appendStr,
    appendHex4,
    petnameFromSeed,
} from "unique-names-generator";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Petname` | `class` | 字段: `state: u64` · 方法: `init, nextU64, generate, generateSlug, generateSubdomain` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `splitmix64Next` | `(state: u64): u64` | 调用 `splitmix64Next` 并返回 `u64`。 |
| `adjectiveAt` | `(idx: usize): string` | 调用 `adjectiveAt` 并返回 `string`。 |
| `adverbAt` | `(idx: usize): string` | 调用 `adverbAt` 并返回 `string`。 |
| `nounAt` | `(idx: usize): string` | 调用 `nounAt` 并返回 `string`。 |
| `appendStr` | `(out: []u8, pos: usize, s: string): usize` | 调用 `appendStr` 并返回 `usize`。 |
| `appendHex4` | `(out: []u8, pos: usize, val: u64): usize` | 调用 `appendHex4` 并返回 `usize`。 |
| `petnameFromSeed` | `(seed: u64, words: u8, sep: string, out: []u8): string` | 调用 `petnameFromSeed` 并返回 `string`。 |
| `subdomainFromSeed` | `(seed: u64, out: []u8): string` | 调用 `subdomainFromSeed` 并返回 `string`。 |

## 最小调用示例

```cp
import { appendStr } from "unique-names-generator";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = appendStr(out_buf[0..256], 0, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
