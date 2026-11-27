[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# terser

Zero-allocation lexer-safe JS/TS/CSS code minifier (npm terser parity) preserving strings, templates, legal comments, keyword spacing, and unary operator separation for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:terser` ([https://github.com/terser/terser](https://github.com/terser/terser))（原库名 `terser` → CP 包名 `terser`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `terser` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add terser
```

```cp
import {
    MinifyOptions,
    MinifyStats,
    tmIsSpace,
    tmIsWord,
    tmNeedsJsSpace,
    tmContainsSubstr,
    tmIsLegalComment,
    tmIsCssPunct,
} from "terser";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MinifyOptions` | `class` | 字段: `strip_comments: bool, preserve_legal_comments: bool, collapse_whitespace: bool, keep_newlines: bool` · 方法: `defaults` |
| `MinifyStats` | `class` | 字段: `input_len: usize, output_len: usize, comments_removed: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `tmIsSpace` | `(ch: u8): bool` | 调用 `tmIsSpace` 并返回 `bool`。 |
| `tmIsWord` | `(ch: u8): bool` | 调用 `tmIsWord` 并返回 `bool`。 |
| `tmNeedsJsSpace` | `(prev: u8, next: u8): bool` | 调用 `tmNeedsJsSpace` 并返回 `bool`。 |
| `tmContainsSubstr` | `(s: string, needle: string): bool` | 调用 `tmContainsSubstr` 并返回 `bool`。 |
| `tmIsLegalComment` | `(comment: string): bool` | 调用 `tmIsLegalComment` 并返回 `bool`。 |
| `tmIsCssPunct` | `(ch: u8): bool` | 调用 `tmIsCssPunct` 并返回 `bool`。 |
| `minifyJs` | `(input: string, opts: MinifyOptions, out: []u8): MinifyStats` | 调用 `minifyJs` 并返回 `MinifyStats`。 |
| `minifyCss` | `(input: string, out: []u8): MinifyStats` | 调用 `minifyCss` 并返回 `MinifyStats`。 |

## 最小调用示例

```cp
import { tmContainsSubstr } from "terser";

pub fn main() {
    let result = tmContainsSubstr("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
