[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bytesize

Zero-heap integer byte size constants (SI & IEC), ByteSize builder, parser, and human-readable formatter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bytesize` ([https://github.com/hyunsik/bytesize](https://github.com/hyunsik/bytesize))

| 元信息 | 值 |
| --- | --- |
| 包名 | `bytesize` |
| 版本 | `1.0.0` |
| 许可证 | `Apache-2.0` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bytesize
```

```cp
import {
    ByteSize,
    bszWriteByte,
    bszUnitMultiplier,
    bszFormatScaled,
    formatSi,
    formatIec,
    parseByteSize,
} from "bytesize";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ByteSize` | `class` | 字段: `val: u64` · 方法: `b, kb, mb, gb, tb, pb, +10 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bszWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `bszWriteByte` 并返回 `usize`。 |
| `bszUnitMultiplier` | `(s: string): u64?` | 调用 `bszUnitMultiplier` 并返回 `u64?`。 |
| `bszFormatScaled` | `(bytes: u64, unit: u64, suffix: string, out: []u8): usize` | 调用 `bszFormatScaled` 并返回 `usize`。 |
| `formatSi` | `(bytes: u64, out: []u8): usize` | 通过 `formatSi` 编码或格式化输出，返回 `usize`。 |
| `formatIec` | `(bytes: u64, out: []u8): usize` | 通过 `formatIec` 编码或格式化输出，返回 `usize`。 |
| `parseByteSize` | `(input: string): u64?` | 通过 `parseByteSize` 解析输入数据，返回 `u64?`。 |

## 最小调用示例

```cp
import { parseByteSize } from "bytesize";

pub fn main() {
    let result = parseByteSize("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
