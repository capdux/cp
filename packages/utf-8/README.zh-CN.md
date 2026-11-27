[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# utf-8

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-copy incremental streaming UTF-8 decoder and RFC 3629 validator with Unicode Maximal Subpart lossy recovery in pure CP

## 安装与导入

```bash
cap add utf-8
```

```cp
import { u8dExpectedLen } from "utf-8";
```

## 核心导出 API

- `export enum Utf8DecodeStatus`
- `export fn u8dExpectedLen(b0: u8): usize;`
- `export fn u8dIsValidSecond(b0: u8, b1: u8): bool;`
- `export fn u8dIsValidCont(b0: u8, idx: usize, b: u8): bool;`
- `export fn u8dWriteSlice(out: []u8, pos: usize, src: &[]u8): usize;`
- `export fn u8dWriteReplacement(out: []u8, pos: usize): usize;`
- `export fn decodeUtf8Chunk(input: &[]u8): Utf8ChunkResult;`
- `export fn isValidUtf8(input: &[]u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`utf-8@1.0.0`)，许可证：MIT OR Apache-2.0
