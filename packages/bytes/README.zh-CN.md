[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bytes

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Unified zero-heap byte size parser/formatter (IEC KiB/MiB/GiB & SI KB/MB/GB) and zero-copy byte buffer cursors (BytesReader/BytesMut) for CP (superset of npm bytes & crates.io bytes)

## 安装与导入

```bash
cap add bytes
```

```cp
import { bytToLower } from "bytes";
```

## 核心导出 API

- `export fn bytToLower(c: u8): u8;`
- `export fn bytIsSpace(c: u8): bool;`
- `export fn bytIsDigit(c: u8): bool;`
- `export fn bytUnitMultiplier(unit: string, binary_default: bool): u64;`
- `export fn bytU64Len(val: u64): usize;`
- `export fn parseBytes(input: string, binary_default: bool): i64;`
- `export fn parse(input: string): i64;`
- `export fn formatBytes(bytes: u64, binary: bool, decimals: u32, out: []u8): usize;`
- `export fn formatIec(bytes: u64, out: []u8): usize;`
- `export fn formatSi(bytes: u64, out: []u8): usize;`
- `export fn bytbCopyBytes(dst: []u8, src: &[]u8, n: usize);`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`bytes@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`bytes@1.0.0`)，许可证：MIT
