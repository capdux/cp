[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# trim-newlines

面向 CP 语言的零分配、零拷贝首尾换行符（`\r` / `\n`）裁剪工具，对齐 npm [`trim-newlines`](https://github.com/sindresorhus/trim-newlines) v5。

## 源包对齐说明

- **上游源包**：npm `trim-newlines` v5（`sindresorhus/trim-newlines`，MIT）
- **CP 实现**：`trim-newlines` v1.0.0，纯 CP 净室 MIT 实现
- **架构特点**：100% 零拷贝子切片操作——仅移除首尾 CR（`\r`）与 LF（`\n`）字节，完整保留首尾空格、制表符以及字符串内部的换行符。

## 安装与导入

```bash
cap add trim-newlines
```

```cp
import {
    trimNewlines,
    trimNewlinesStart,
    trimNewlinesEnd,
    countLeadingNewlines,
    countTrailingNewlines,
} from "trim-newlines";
```

## 核心 API

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `trimNewlines` | `fn trimNewlines(input: string): string` | 零拷贝移除首尾所有 `\r` 与 `\n` 字节并返回子切片 |
| `trimNewlinesStart` | `fn trimNewlinesStart(input: string): string` | 零拷贝移除开头所有 `\r` 与 `\n` 字节 |
| `trimNewlinesEnd` | `fn trimNewlinesEnd(input: string): string` | 零拷贝移除末尾所有 `\r` 与 `\n` 字节 |
| `countLeadingNewlines` | `fn countLeadingNewlines(input: string): usize` | 统计开头连续 `\r` / `\n` 字节数 |
| `countTrailingNewlines` | `fn countTrailingNewlines(input: string): usize` | 统计末尾连续 `\r` / `\n` 字节数 |

## 使用示例

### 1. 裁剪开头、末尾或两端换行

```cp
import { trimNewlines, trimNewlinesStart, trimNewlinesEnd } from "trim-newlines";
import { print } from "cap:io";

pub fn main() {
    print(trimNewlines("\r\n  hello\nworld  \r\n")); // "  hello\nworld  "
    print(trimNewlinesStart("\n\nunicorn\n"));       // "unicorn\n"
    print(trimNewlinesEnd("\nunicorn\r\n\n"));       // "\nunicorn"
}
```

### 2. 统计首尾换行字节数

```cp
import { countLeadingNewlines, countTrailingNewlines } from "trim-newlines";

pub fn inspectPadding(text: string): usize {
    const lead = countLeadingNewlines(text);   // "\r\nfoo\n" -> 2
    const trail = countTrailingNewlines(text); // "\r\nfoo\n" -> 1
    return lead + trail;
}
```

### 3. 判断全换行字符串

```cp
import { trimNewlines } from "trim-newlines";

pub fn isOnlyNewlines(text: string): bool {
    return trimNewlines(text).len == 0;
}
```

## 与源包差异与安全亮点

- **真零拷贝**：直接返回输入字符串的子切片，复杂度 O(k)（仅扫描首尾换行字节），零堆分配。
- **保留水平空白**：与通用 `trim()` 不同，不会误删首尾空格（`' '`）和制表符（`'\t'`）。
- **内置计数工具**：额外提供 `countLeadingNewlines` 与 `countTrailingNewlines`，便于行号与偏移计算。
