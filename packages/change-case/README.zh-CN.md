[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# change-case

Zero-heap full-featured case conversion library covering all 12 change-case v5 styles (camel, pascal, snake, kebab, constant, dot, path, train, capital, sentence, no, swap).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:change-case` ([https://github.com/blakeembrey/change-case](https://github.com/blakeembrey/change-case))

| 元信息 | 值 |
| --- | --- |
| 包名 | `change-case` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add change-case
```

```cp
import {
    ccaseIsUpper,
    ccaseIsLower,
    ccaseIsDigit,
    ccaseIsAlnum,
    ccaseToUpper,
    ccaseToLower,
    ccaseJoinLower,
    ccaseCapitalizeWords,
} from "change-case";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ccaseIsUpper` | `(c: u8): bool` | 调用 `ccaseIsUpper` 并返回 `bool`。 |
| `ccaseIsLower` | `(c: u8): bool` | 调用 `ccaseIsLower` 并返回 `bool`。 |
| `ccaseIsDigit` | `(c: u8): bool` | 调用 `ccaseIsDigit` 并返回 `bool`。 |
| `ccaseIsAlnum` | `(c: u8): bool` | 调用 `ccaseIsAlnum` 并返回 `bool`。 |
| `ccaseToUpper` | `(c: u8): u8` | 调用 `ccaseToUpper` 并返回 `u8`。 |
| `ccaseToLower` | `(c: u8): u8` | 调用 `ccaseToLower` 并返回 `u8`。 |
| `ccaseJoinLower` | `(input: string, sep: string, out: []u8): usize` | 调用 `ccaseJoinLower` 并返回 `usize`。 |
| `ccaseCapitalizeWords` | `(out: []u8, len: usize, sep_byte: u8)` | 就地执行 `ccaseCapitalizeWords` 操作。 |
| `camelCase` | `(input: string, out: []u8): usize` | 调用 `camelCase` 并返回 `usize`。 |
| `pascalCase` | `(input: string, out: []u8): usize` | 调用 `pascalCase` 并返回 `usize`。 |
| `snakeCase` | `(input: string, out: []u8): usize` | 调用 `snakeCase` 并返回 `usize`。 |
| `kebabCase` | `(input: string, out: []u8): usize` | 调用 `kebabCase` 并返回 `usize`。 |
| `constantCase` | `(input: string, out: []u8): usize` | 调用 `constantCase` 并返回 `usize`。 |
| `dotCase` | `(input: string, out: []u8): usize` | 调用 `dotCase` 并返回 `usize`。 |
| `pathCase` | `(input: string, out: []u8): usize` | 调用 `pathCase` 并返回 `usize`。 |
| `trainCase` | `(input: string, out: []u8): usize` | 调用 `trainCase` 并返回 `usize`。 |
| `capitalCase` | `(input: string, out: []u8): usize` | 调用 `capitalCase` 并返回 `usize`。 |
| `sentenceCase` | `(input: string, out: []u8): usize` | 调用 `sentenceCase` 并返回 `usize`。 |
| `noCase` | `(input: string, out: []u8): usize` | 调用 `noCase` 并返回 `usize`。 |
| `swapCase` | `(input: string, out: []u8): usize` | 调用 `swapCase` 并返回 `usize`。 |

## 最小调用示例

```cp
import { ccaseJoinLower } from "change-case";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = ccaseJoinLower("example", "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
