[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# zip

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap PKZIP Stored archive writer, local header & EOCD parser, and CRC-32 verifier (crates.io zip equivalent)

## 安装与导入

```bash
cap add zip
```

```cp
import { zipEmptyLocalHeader } from "zip";
```

## 核心导出 API

- `export fn zipEmptyLocalHeader(): ZipLocalHeader;`
- `export fn zipEmptyEocd(): ZipEocd;`
- `export fn zipCopyStr(dst: []u8, offset: usize, s: string);`
- `export fn zipCopyBytes(dst: []u8, offset: usize, src: []u8);`
- `export fn writeStoredZipSingle(filename: string, payload: []u8, out: []u8): usize?;`
- `export fn parseLocalHeader(buf: []u8, offset: usize): ZipLocalHeader;`
- `export fn findAndParseEocd(buf: []u8): ZipEocd;`
- `export fn verifyStoredEntry(buf: []u8, hdr: ZipLocalHeader): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`zip@1.0.0`)，许可证：MIT
