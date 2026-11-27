[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# zstd

> 版本：`1.0.0` | 许可证：`BSD-3-Clause` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 zstd

## 安装与导入

```bash
cap add zstd
```

```cp
import { ZSTD_versionNumber } from "zstd";
```

## 核心导出 API

- `export fn ZSTD_versionNumber(): u32;`
- `export fn ZSTD_versionString(): [*:0]&u8!;`
- `export fn ZSTD_compressBound(srcSize: usize): usize;`
- `export fn ZSTD_isError(code: usize): u32;`
- `export fn ZSTD_getErrorName(code: usize): [*:0]&u8!;`
- `export fn ZSTD_getFrameContentSize(src: [*]&u8!, srcSize: usize): u64;`
- `export fn ZSTD_createCCtx(): *ZSTD_CCtx!?;`
- `export fn ZSTD_freeCCtx(cctx: *ZSTD_CCtx!): usize;`
- `export fn ZSTD_compressCCtx(cctx: *ZSTD_CCtx!, dst: [*]u8!, dstCapacity: usize, src: [*]&u8!, srcSize: usize, compressionLevel: i32): usize;`
- `export fn ZSTD_createDCtx(): *ZSTD_DCtx!?;`
- `export fn ZSTD_freeDCtx(dctx: *ZSTD_DCtx!): usize;`
- `export fn ZSTD_decompressDCtx(dctx: *ZSTD_DCtx!, dst: [*]u8!, dstCapacity: usize, src: [*]&u8!, srcSize: usize): usize;`
- `export fn versionNumber(): u32;`
- `export fn compressBound(src_size: usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`zstd@1.0.0`)，许可证：MIT
