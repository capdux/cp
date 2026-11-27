[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# serde_qs

Zero-allocation nested bracket query string parser & serializer (crates.io serde_qs v0.13 parity) composing percent-encoding for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:serde_qs` ([https://github.com/samscott89/serde_qs](https://github.com/samscott89/serde_qs))（原库名 `serde_qs` → CP 包名 `serde_qs`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `serde_qs` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add serde_qs
```

```cp
import {
    QsPair,
    QsPath,
    sqsKeyEncodeSet,
    parseKeyPath,
    parseQs,
    getQsValueDecoded,
    serializeQs,
} from "serde_qs";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `QsPair` | `class` | 字段: `key: string, value: string` |
| `QsPath` | `class` | 字段: `segments: [8]string, depth: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sqsKeyEncodeSet` | `(): AsciiSet` | 调用 `sqsKeyEncodeSet` 并返回 `AsciiSet`。 |
| `parseKeyPath` | `(key: string): QsPath` | 通过 `parseKeyPath` 解析输入数据，返回 `QsPath`。 |
| `parseQs` | `(query: string, out_pairs: []QsPair): usize` | 通过 `parseQs` 解析输入数据，返回 `usize`。 |
| `getQsValueDecoded` | `(query: string, target_key: string, out: []u8): usize?` | 查询或读取目标数据，返回 `usize?`。 |
| `serializeQs` | `(pairs: &[]QsPair, out: []u8): usize` | 调用 `serializeQs` 并返回 `usize`。 |

## 最小调用示例

```cp
import { parseKeyPath } from "serde_qs";

pub fn main() {
    let result = parseKeyPath("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
