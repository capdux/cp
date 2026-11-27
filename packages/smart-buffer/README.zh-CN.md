[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# smart-buffer

> 版本：`4.2.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap cursor-based binary reader/writer buffer with big/little-endian integer and string operations

## 安装与导入

```bash
cap add smart-buffer
```

```cp
import { sbufMin } from "smart-buffer";
```

## 核心导出 API

- `export fn sbufMin(a: usize, b: usize): usize;`
- `export fn sbufW16BE(dst: []u8, off: usize, v: u16): bool;`
- `export fn sbufW16LE(dst: []u8, off: usize, v: u16): bool;`
- `export fn sbufW32BE(dst: []u8, off: usize, v: u32): bool;`
- `export fn sbufW32LE(dst: []u8, off: usize, v: u32): bool;`
- `export fn sbufR16BE(src: &[]u8, off: usize): u16?;`
- `export fn sbufR16LE(src: &[]u8, off: usize): u16?;`
- `export fn sbufR32BE(src: &[]u8, off: usize): u32?;`
- `export fn sbufR32LE(src: &[]u8, off: usize): u32?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`smart-buffer@4.2.0`)（https://github.com/JoshGlazebrook/smart-buffer），许可证：MIT
