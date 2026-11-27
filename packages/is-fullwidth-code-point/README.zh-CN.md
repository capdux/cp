[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# is-fullwidth-code-point

纯 CP 实现的零分配 UAX #11 Unicode 全角码点判断与 UTF-8 终端显示宽度计算工具库。

- **源包对齐**：对标 `npm` `is-fullwidth-code-point` `v5`（MIT，sindresorhus/is-fullwidth-code-point）
- **包版本**：`1.0.0`
- **内存模型**：全程零堆分配，纯码点区间查表与零拷贝 UTF-8 切片。

## 安装与导入

```bash
cap add is-fullwidth-code-point
```

```cp
import {
    isFullwidthCodePoint,
    isZeroWidthCodePoint,
    codePointWidth,
    countFullwidthChars,
    hasFullwidthChar,
    stringDisplayWidth,
    truncateToDisplayWidth,
} from "is-fullwidth-code-point";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `isFullwidthCodePoint` | `fn isFullwidthCodePoint(cp: u32): bool` | 按 UAX #11 判断 Unicode 码点 `cp` 是否为东亚宽字符/全角字符（占 2 列）。 |
| `isZeroWidthCodePoint` | `fn isZeroWidthCodePoint(cp: u32): bool` | 判断 `cp` 是否为控制字符、组合标记或零宽字符（占 0 列）。 |
| `codePointWidth` | `fn codePointWidth(cp: u32): usize` | 返回单个码点的终端列宽：`0`、`1` 或 `2`。 |
| `countFullwidthChars` | `fn countFullwidthChars(s: string): usize` | 统计 UTF-8 字符串 `s` 中的全角字符个数。 |
| `hasFullwidthChar` | `fn hasFullwidthChar(s: string): bool` | 判断 `s` 是否包含至少一个全角字符。 |
| `stringDisplayWidth` | `fn stringDisplayWidth(s: string): usize` | 计算 UTF-8 字符串 `s` 在终端中的总显示列宽。 |
| `truncateToDisplayWidth` | `fn truncateToDisplayWidth(s: string, max_width: usize): string` | 返回显示宽度不超过 `max_width` 的最长 UTF-8 前缀切片，绝不截断多字节字符。 |

## 使用示例

### 1. 码点宽度判断

```cp
import { isFullwidthCodePoint, codePointWidth } from "is-fullwidth-code-point";

fn checkChars(): void {
    const is_cjk = isFullwidthCodePoint(0x4E2D); // true ('中')
    const is_ascii = isFullwidthCodePoint(0x41); // false ('A')
    const w = codePointWidth(0x1F600);           // 2（Emoji）
}
```

### 2. UTF-8 终端列宽计算与安全截断

```cp
import { stringDisplayWidth, truncateToDisplayWidth, hasFullwidthChar } from "is-fullwidth-code-point";

fn layoutDemo(): void {
    const label = "CP语言2026";
    const width = stringDisplayWidth(label);           // 10 (2 + 2*2 + 4)
    const has_fw = hasFullwidthChar(label);            // true
    const clipped = truncateToDisplayWidth(label, 5);  // "CP语"（宽 4，不拆裂 '言'）
}
```

## 与源包差异与安全亮点

- **开箱即用的字符串列宽工具**：在单码点 `isFullwidthCodePoint` 基础上，扩展了 `isZeroWidthCodePoint`、`codePointWidth`、`stringDisplayWidth` 与 `truncateToDisplayWidth`。
- **零拷贝边界安全截断**：`truncateToDisplayWidth` 严格按 UTF-8 码点边界返回子切片，零堆分配且不产生非法 UTF-8。
