[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# isbn

Zero-allocation ISO 2108 ISBN-10, ISBN-13, ISSN, and EAN-13 validator and converter for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:isbn` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

| 元信息 | 值 |
| --- | --- |
| 包名 | `isbn` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add isbn
```

```cp
import {
    isbnToUpper,
    isbnIsDigit,
    stripHyphens,
    isbnNormalizeExact,
    computeIsbn13CheckDigit,
    isValidIsbn10,
    isValidEan13,
    isValidIsbn13,
} from "isbn";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isbnToUpper` | `(c: u8): u8` | 执行 `isbnToUpper` 条件判定，返回 `bool`。 |
| `isbnIsDigit` | `(c: u8): bool` | 执行 `isbnIsDigit` 条件判定，返回 `bool`。 |
| `stripHyphens` | `(input: string, out: []u8): usize` | 调用 `stripHyphens` 并返回 `usize`。 |
| `isbnNormalizeExact` | `(input: string, expected_len: usize, out: []u8): bool` | 执行 `isbnNormalizeExact` 条件判定，返回 `bool`。 |
| `computeIsbn13CheckDigit` | `(prefix12: string): u8` | 调用 `computeIsbn13CheckDigit` 并返回 `u8`。 |
| `isValidIsbn10` | `(input: string): bool` | 执行 `isValidIsbn10` 条件判定，返回 `bool`。 |
| `isValidEan13` | `(input: string): bool` | 执行 `isValidEan13` 条件判定，返回 `bool`。 |
| `isValidIsbn13` | `(input: string): bool` | 执行 `isValidIsbn13` 条件判定，返回 `bool`。 |
| `isValidIssn` | `(input: string): bool` | 执行 `isValidIssn` 条件判定，返回 `bool`。 |
| `isValidIsbn` | `(input: string): bool` | 执行 `isValidIsbn` 条件判定，返回 `bool`。 |
| `isbn10To13` | `(isbn10: string, out: []u8): usize` | 执行 `isbn10To13` 条件判定，返回 `bool`。 |
| `isbn13To10` | `(isbn13: string, out: []u8): usize` | 执行 `isbn13To10` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isbnNormalizeExact } from "isbn";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = isbnNormalizeExact("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
