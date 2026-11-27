[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# mime_guess

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation file path and extension MIME type guesser for CP (clean-room MIT, inspired by Rust mime_guess v2, powered by mime-types).

## 安装与导入

```bash
cap add mime_guess
```

```cp
import { mgsStartsWith } from "mime_guess";
```

## 核心导出 API

- `export fn mgsStartsWith(s: string, prefix: string): bool;`
- `export fn mgsHasPathSep(s: string): bool;`
- `export fn mgsIsCompressibleMime(mime_type: string): bool;`
- `export fn extractExt(path: string): string?;`
- `export fn getExtForMime(mime_type: string): string?;`
- `export fn getCharsetForMime(mime_type: string): string?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`mime_guess@1.0.0`)，许可证：MIT
