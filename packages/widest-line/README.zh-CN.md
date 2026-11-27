[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# widest-line

Zero-allocation widest visual terminal line width calculator (npm widest-line parity) composing string-width for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:widest-line` ([https://github.com/sindresorhus/widest-line](https://github.com/sindresorhus/widest-line))

| 元信息 | 值 |
| --- | --- |
| 包名 | `widest-line` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add widest-line
```

```cp
import {
    wlTrimCr,
    wlLineWidthWithTab,
    countLines,
    widestLine,
    widestLineWithOptions,
    lineWidths,
    lineWidthsWithOptions,
    widestLineWithTab,
} from "widest-line";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `wlTrimCr` | `(line: string): string` | 调用 `wlTrimCr` 并返回 `string`。 |
| `wlLineWidthWithTab` | `(line: string, tab_width: usize): usize` | 调用 `wlLineWidthWithTab` 并返回 `usize`。 |
| `countLines` | `(s: string): usize` | 调用 `countLines` 并返回 `usize`。 |
| `widestLine` | `(s: string): usize` | 调用 `widestLine` 并返回 `usize`。 |
| `widestLineWithOptions` | `(s: string, opts: StringWidthOptions): usize` | 调用 `widestLineWithOptions` 并返回 `usize`。 |
| `lineWidths` | `(s: string, out: []usize): usize` | 调用 `lineWidths` 并返回 `usize`。 |
| `lineWidthsWithOptions` | `(s: string, opts: StringWidthOptions, out: []usize): usize` | 调用 `lineWidthsWithOptions` 并返回 `usize`。 |
| `widestLineWithTab` | `(s: string, tab_width: usize): usize` | 调用 `widestLineWithTab` 并返回 `usize`。 |

## 最小调用示例

```cp
import { wlTrimCr } from "widest-line";

pub fn main() {
    let result = wlTrimCr("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
