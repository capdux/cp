[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/boxen

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:boxen` ([https://github.com/sindresorhus/boxen](https://github.com/sindresorhus/boxen))

零堆分配的 CP CLI 终端边框盒子渲染引擎 — 洁净室 MIT 实现，对标 npm `boxen`。

Zero-allocation CLI terminal box renderer for CP — clean-room MIT parity with npm `boxen`.

## 特性 / Features

- **6 种边框样式 / 6 Border Styles**：`Single`（`┌─┐│└┘`）、`Double`（`╔═╗║╚╝`）、`Round`（`╭─╮│╰╯`）、`Bold`（`┏━┓┃┗┛`）、`Classic`（`+-+|+-+`）、`SingleDouble`（`╓─╖║╙╜`）。
- **标题嵌入与对齐 / Embedded Title & Alignment**：支持顶边框嵌入标题（如 `╭─ Status ───╮`），正文与标题均支持 `Left` / `Center` / `Right` 对齐，以及 `padding_x` / `padding_y` / `margin_left`。
- **CJK 双宽与 ANSI 穿透 / CJK & ANSI Aware**：准确计算中文/全角/Emoji 宽字符宽度（宽度 2）并忽略 ANSI 颜色转义序列（`\x1b[...m`），保证多行右边框像素级对齐。
- **零堆分配 / Zero Heap Allocation**：全部写入调用方 `out: []u8` 缓冲区。

## 安装 / Install

```bash
cap add @cap/boxen
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `BorderStyle` | 枚举：`Single` / `Double` / `Round` / `Bold` / `Classic` / `SingleDouble` |
| `TextAlign` | 枚举：`Left` / `Center` / `Right` |
| `BoxOptions` | `{ style, text_align, padding_x, padding_y, margin_left, title, title_align }` + `defaults()` |
| `renderBox(text, opts, out)` | 使用自定义选项渲染终端盒子 |
| `box(text, out)` | 使用默认单线边框快速渲染 |
| `titledBox(title, text, style, out)` | 快速渲染带顶边框标题的盒子 |
| `displayWidth(s)` | 计算单行终端显示宽度（CJK 宽 2、忽略 ANSI 转义） |

## 使用示例 / Usage

```cp
import { box, titledBox, renderBox, BoxOptions, BorderStyle, TextAlign } from "@cap/boxen";

let buf: [1024]u8 = undefined;
const out = titledBox("Status", "CP Runtime Ready!\nAll 100+ packages active", BorderStyle.Round, buf[0..1024]);
```

输出效果 / Output:

```text
╭─ Status ─────────────────╮
│ CP Runtime Ready!        │
│ All 100+ packages active │
╰──────────────────────────╯
```

## License

MIT
