[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# boxen

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CLI terminal box renderer with 6 Unicode/ASCII border styles, embedded titles, alignment, CJK double-width & ANSI escape awareness (boxen parity) for CP

## 安装与导入

```bash
cap add boxen
```

```cp
import { borderForStyle } from "boxen";
```

## 核心导出 API

- `export enum BorderStyle`
- `export enum TextAlign`
- `export fn borderForStyle(style: BorderStyle): BorderChars;`
- `export fn decodeUtf8At(s: string, i: usize): Utf8Step;`
- `export fn codepointWidth(cp: u32): usize;`
- `export fn displayWidth(s: string): usize;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushStr(out: []u8, pos: usize, s: string): usize;`
- `export fn pushRepeat(out: []u8, pos: usize, s: string, count: usize): usize;`
- `export fn pushSpaces(out: []u8, pos: usize, count: usize): usize;`
- `export fn renderBox(text: string, opts: BoxOptions, out: []u8): string;`
- `export fn box(text: string, out: []u8): string;`
- `export fn titledBox(title: string, text: string, style: BorderStyle, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`boxen@1.0.0`)，许可证：MIT
