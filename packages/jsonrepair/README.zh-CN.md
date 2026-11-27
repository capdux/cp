[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# jsonrepair

Zero-allocation malformed & LLM-generated JSON repair engine (jsonrepair / dirty-json / Rust llm_json parity: strips markdown fences, fixes single quotes, unquoted keys, trailing commas, Python True/False/None & truncated brackets) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jsonrepair` ([https://github.com/josdejong/jsonrepair](https://github.com/josdejong/jsonrepair))（原库名 `jsonrepair` → CP 包名 `jsonrepair`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `jsonrepair` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add jsonrepair
```

```cp
import {
    isIdentStart,
    isIdentChar,
    appendByte,
    stripMarkdownFence,
    skipWsAndComments,
    repairJson,
} from "jsonrepair";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isIdentStart` | `(c: u8): bool` | 执行 `isIdentStart` 条件判定，返回 `bool`。 |
| `isIdentChar` | `(c: u8): bool` | 执行 `isIdentChar` 条件判定，返回 `bool`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `stripMarkdownFence` | `(raw: string): string` | 调用 `stripMarkdownFence` 并返回 `string`。 |
| `skipWsAndComments` | `(s: string, start: usize): usize` | 调用 `skipWsAndComments` 并返回 `usize`。 |
| `repairJson` | `(raw: string, out: []u8): string` | 调用 `repairJson` 并返回 `string`。 |

## 最小调用示例

```cp
import { stripMarkdownFence } from "jsonrepair";

pub fn main() {
    let result = stripMarkdownFence("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
