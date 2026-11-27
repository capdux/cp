[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# indoc

Zero-allocation multiline string dedent, indent, stripMargin, and newline normalization engine for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:indoc` ([https://github.com/dtolnay/indoc](https://github.com/dtolnay/indoc))

| 元信息 | 值 |
| --- | --- |
| 包名 | `indoc` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add indoc
```

```cp
import {
    isHws,
    isLineBlank,
    appendByte,
    stripOuterSingleBlankLines,
    trimBlankLines,
    minCommonIndent,
    dedent,
    indent,
} from "indoc";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isHws` | `(c: u8): bool` | 执行 `isHws` 条件判定，返回 `bool`。 |
| `isLineBlank` | `(s: string, start: usize, end: usize): bool` | 执行 `isLineBlank` 条件判定，返回 `bool`。 |
| `appendByte` | `(out: []u8, pos: usize, c: u8): usize` | 调用 `appendByte` 并返回 `usize`。 |
| `stripOuterSingleBlankLines` | `(input: string): string` | 调用 `stripOuterSingleBlankLines` 并返回 `string`。 |
| `trimBlankLines` | `(input: string): string` | 调用 `trimBlankLines` 并返回 `string`。 |
| `minCommonIndent` | `(input: string): usize` | 调用 `minCommonIndent` 并返回 `usize`。 |
| `dedent` | `(input: string, out: []u8): string` | 调用 `dedent` 并返回 `string`。 |
| `indent` | `(input: string, prefix: string, skip_empty: bool, out: []u8): string` | 调用 `indent` 并返回 `string`。 |
| `stripMargin` | `(input: string, margin_char: u8, out: []u8): string` | 调用 `stripMargin` 并返回 `string`。 |
| `normalizeNewlines` | `(input: string, out: []u8): string` | 调用 `normalizeNewlines` 并返回 `string`。 |

## 最小调用示例

```cp
import { isLineBlank } from "indoc";

pub fn main() {
    let result = isLineBlank("example", 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
