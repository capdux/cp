[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unicode-width

Zero-allocation Unicode 15.1 East Asian Width & terminal display column width calculator with UTF-8 safe truncation for CP.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-width` ([https://github.com/unicode-rs/unicode-width](https://github.com/unicode-rs/unicode-width)) / `npm:eastasianwidth` ([https://github.com/komagata/eastasianwidth](https://github.com/komagata/eastasianwidth)) / `npm:is-fullwidth-code-point` ([https://github.com/sindresorhus/is-fullwidth-code-point](https://github.com/sindresorhus/is-fullwidth-code-point)) / `npm:wcwidth` ([https://github.com/timoxley/wcwidth](https://github.com/timoxley/wcwidth))

| 元信息 | 值 |
| --- | --- |
| 包名 | `unicode-width` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add unicode-width
```

```cp
import {
    uwNextCodepoint,
    uwIsZeroWidth,
    uwIsWide,
    uwIsAmbiguous,
    charWidth,
    charWidthCjk,
    strWidth,
    strWidthCjk,
} from "unicode-width";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `uwNextCodepoint` | `(s: &[]u8, pos: usize, next_pos: &mut usize): u32` | 调用 `uwNextCodepoint` 并返回 `u32`。 |
| `uwIsZeroWidth` | `(cp: u32): bool` | 调用 `uwIsZeroWidth` 并返回 `bool`。 |
| `uwIsWide` | `(cp: u32): bool` | 调用 `uwIsWide` 并返回 `bool`。 |
| `uwIsAmbiguous` | `(cp: u32): bool` | 调用 `uwIsAmbiguous` 并返回 `bool`。 |
| `charWidth` | `(cp: u32): usize` | 调用 `charWidth` 并返回 `usize`。 |
| `charWidthCjk` | `(cp: u32): usize` | 调用 `charWidthCjk` 并返回 `usize`。 |
| `strWidth` | `(s: string): usize` | 调用 `strWidth` 并返回 `usize`。 |
| `strWidthCjk` | `(s: string): usize` | 调用 `strWidthCjk` 并返回 `usize`。 |
| `truncateToWidth` | `(s: string, max_cols: usize): usize` | 调用 `truncateToWidth` 并返回 `usize`。 |

## 最小调用示例

```cp
import { strWidth } from "unicode-width";

pub fn main() {
    let result = strWidth("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
