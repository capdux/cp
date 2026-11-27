[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:trim-newlines` ([https://github.com/sindresorhus/trim-newlines](https://github.com/sindresorhus/trim-newlines)) | 许可：`MIT`

# `trim-newlines`

零堆分配、零拷贝纯 CP 字符串首尾换行符（`\r` / `\n`）裁剪库，完整对标 npm Top-700 包 `trim-newlines` v5。

## 核心 API

- `trimNewlines(input: string): string` — 裁剪首尾全部 `\r` / `\n`（保留空格、Tab 与中间换行）
- `trimNewlinesStart(input: string): string` — 仅裁剪前导 `\r` / `\n`
- `trimNewlinesEnd(input: string): string` — 仅裁剪尾部 `\r` / `\n`
- `countLeadingNewlines(input: string): usize` — 统计前导换行字节数
- `countTrailingNewlines(input: string): usize` — 统计尾部换行字节数

## 示例

```typescript
import { trimNewlines, trimNewlinesStart, trimNewlinesEnd } from "trim-newlines";

const a = trimNewlines("\r\n\nhello\nworld\r\n"); // "hello\nworld"
const b = trimNewlinesStart("\n\n  cp  \n");      // "  cp  \n"
const c = trimNewlinesEnd("\n  cp  \r\n");         // "\n  cp  "
```
