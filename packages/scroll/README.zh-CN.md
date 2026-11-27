[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# scroll

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation endian-aware offset-advancing binary reader/writer cursors (PreadCursor & PwriteCursor) in pure CP

## 安装与导入

```bash
cap add scroll
```

```cp
import { scrlReadU16 } from "scroll";
```

## 核心导出 API

- `export enum Endian`
- `export fn scrlReadU16(buf: &[]u8, off: usize, endian: Endian): u16;`
- `export fn scrlReadU32(buf: &[]u8, off: usize, endian: Endian): u32;`
- `export fn scrlReadU64(buf: &[]u8, off: usize, endian: Endian): u64;`
- `export fn scrlWriteU16(buf: []u8, off: usize, v: u16, endian: Endian);`
- `export fn scrlWriteU32(buf: []u8, off: usize, v: u32, endian: Endian);`
- `export fn scrlWriteU64(buf: []u8, off: usize, v: u64, endian: Endian);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`scroll@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`endian-type@1.0.0`)，许可证：MIT
