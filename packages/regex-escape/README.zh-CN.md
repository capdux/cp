[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# regex-escape

Zero-heap regular expression literal escaper, character-class escaper, unescaper, and metacharacter detector.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:regex-escape` ([https://github.com/rust-lang/regex](https://github.com/rust-lang/regex))

| 元信息 | 值 |
| --- | --- |
| 包名 | `regex-escape` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add regex-escape
```

```cp
import {
    isMetaChar,
    isEscapeTarget,
    needsEscape,
    escapedLen,
    escapeRegex,
    escapeCharClass,
    unescapeRegexLiteral,
} from "regex-escape";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isMetaChar` | `(c: u8): bool` | 执行 `isMetaChar` 条件判定，返回 `bool`。 |
| `isEscapeTarget` | `(c: u8): bool` | 执行 `isEscapeTarget` 条件判定，返回 `bool`。 |
| `needsEscape` | `(input: string): bool` | 调用 `needsEscape` 并返回 `bool`。 |
| `escapedLen` | `(input: string): usize` | 调用 `escapedLen` 并返回 `usize`。 |
| `escapeRegex` | `(input: string, out: []u8): usize` | 调用 `escapeRegex` 并返回 `usize`。 |
| `escapeCharClass` | `(input: string, out: []u8): usize` | 调用 `escapeCharClass` 并返回 `usize`。 |
| `unescapeRegexLiteral` | `(input: string, out: []u8): usize` | 调用 `unescapeRegexLiteral` 并返回 `usize`。 |

## 最小调用示例

```cp
import { needsEscape } from "regex-escape";

pub fn main() {
    let result = needsEscape("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
