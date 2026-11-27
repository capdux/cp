[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# tar

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap POSIX USTAR 512-byte tar archive header codec and streaming cursor (crates.io tar equivalent)

## 安装与导入

```bash
cap add tar
```

```cp
import { tarEmptyHeader } from "tar";
```

## 核心导出 API

- `export fn tarEmptyHeader(): TarHeader;`
- `export fn tarWriteOctal(out: []u8, offset: usize, digits: usize, val: u64): bool;`
- `export fn tarParseOctal(buf: []u8, offset: usize, width: usize, require_digit: bool): u64?;`
- `export fn tarComputeChecksum(block512: []u8): u32;`
- `export fn tarIsUstarMagic(block512: []u8): bool;`
- `export fn isZeroBlock(block512: []u8): bool;`
- `export fn tarPaddedSize(raw_size: u64): u64;`
- `export fn encodeTarHeader(name: string, size: u64, mode: u32, mtime: u64, typeflag: u8, out512: []u8): bool;`
- `export fn parseTarHeader(block512: []u8): TarHeader;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`tar@1.0.0`)，许可证：MIT OR Apache-2.0
