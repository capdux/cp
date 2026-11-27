[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# utf-8

Zero-copy incremental streaming UTF-8 decoder and RFC 3629 validator with Unicode Maximal Subpart lossy recovery in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:utf-8` ([https://github.com/SimonSapin/rust-utf8](https://github.com/SimonSapin/rust-utf8)) — `MIT OR Apache-2.0`

| 元信息 | 值 |
| --- | --- |
| 包名 | `utf-8` |
| 版本 | `1.0.0` |
| 许可证 | `MIT OR Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add utf-8
```

```cp
import {
    Utf8DecodeStatus,
    Utf8ChunkResult,
    LossyUtf8Decoder,
    u8dExpectedLen,
    u8dIsValidSecond,
    u8dIsValidCont,
    u8dWriteSlice,
    u8dWriteReplacement,
} from "utf-8";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `Utf8DecodeStatus` | `Ok, InvalidSequence, Incomplete` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Utf8ChunkResult` | `class` | 字段: `status: Utf8DecodeStatus, valid_prefix: string, consumed: usize, error_len: usize` |
| `LossyUtf8Decoder` | `class` | 字段: `pending: [4]u8, pending_len: usize` · 方法: `new, hasPending, pendingLen, reset, push, finish` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `u8dExpectedLen` | `(b0: u8): usize` | 调用 `u8dExpectedLen` 并返回 `usize`。 |
| `u8dIsValidSecond` | `(b0: u8, b1: u8): bool` | 调用 `u8dIsValidSecond` 并返回 `bool`。 |
| `u8dIsValidCont` | `(b0: u8, idx: usize, b: u8): bool` | 调用 `u8dIsValidCont` 并返回 `bool`。 |
| `u8dWriteSlice` | `(out: []u8, pos: usize, src: &[]u8): usize` | 调用 `u8dWriteSlice` 并返回 `usize`。 |
| `u8dWriteReplacement` | `(out: []u8, pos: usize): usize` | 调用 `u8dWriteReplacement` 并返回 `usize`。 |
| `decodeUtf8Chunk` | `(input: &[]u8): Utf8ChunkResult` | 通过 `decodeUtf8Chunk` 解析输入数据，返回 `Utf8ChunkResult`。 |
| `isValidUtf8` | `(input: &[]u8): bool` | 执行 `isValidUtf8` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { decodeUtf8Chunk } from "utf-8";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeUtf8Chunk(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
