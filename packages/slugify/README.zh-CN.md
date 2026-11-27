[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# slugify

Zero-allocation URL slug generator with UTF-8 Latin-1 / Latin Extended-A diacritic folding and truncation for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`slugify`](https://github.com/simov/slugify)（`MIT`）（包名 `slugify`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `slugify` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add slugify
```

```cp
import {
    slToLower,
    slIsAsciiAlnum,
    slFoldLatin,
    slugifyWithSep,
    slugify,
    slugifyTruncate,
    isValidSlug,
} from "slugify";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `slToLower` | `(c: u8): u8` | 调用 `slToLower` 并返回 `u8`。 |
| `slIsAsciiAlnum` | `(c: u8): bool` | 调用 `slIsAsciiAlnum` 并返回 `bool`。 |
| `slFoldLatin` | `(cp: u32, lowercase: bool, buf: []u8): usize` | 调用 `slFoldLatin` 并返回 `usize`。 |
| `slugifyWithSep` | `(input: string, sep: u8, lowercase: bool, out: []u8): usize` | 调用 `slugifyWithSep` 并返回 `usize`。 |
| `slugify` | `(input: string, out: []u8): usize` | 调用 `slugify` 并返回 `usize`。 |
| `slugifyTruncate` | `(input: string, max_len: usize, out: []u8): usize` | 调用 `slugifyTruncate` 并返回 `usize`。 |
| `isValidSlug` | `(s: string): bool` | 执行 `isValidSlug` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isValidSlug } from "slugify";

pub fn main() {
    let result = isValidSlug("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
