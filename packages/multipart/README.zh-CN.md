[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# multipart

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 7578 multipart/form-data streaming reader, writer, and boundary extractor built on mediatype and unicase (clean-room port of Rust multipart/multer)

## 安装与导入

```bash
cap add multipart
```

```cp
import { mpartIsSpace } from "multipart";
```

## 核心导出 API

- `export fn mpartIsSpace(c: u8): bool;`
- `export fn mpartMatchAt(haystack: string, pos: usize, needle: string): bool;`
- `export fn mpartIsBoundaryAt(body: string, pos: usize, boundary: string): bool;`
- `export fn mpartFindFirstBoundary(body: string, boundary: string): usize;`
- `export fn mpartFindNextBoundary(body: string, start: usize, boundary: string, data_end_out: &mut usize, next_bnd_out: &mut usize): bool;`
- `export fn mpartSkipLineEnd(body: string, pos: &mut usize): bool;`
- `export fn mpartParseDisposition(val: string, part: &mut MultipartPart);`
- `export fn extractBoundary(content_type: string): string?;`
- `export fn formatContentTypeHeader(boundary: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`multipart@1.0.0`)，许可证：MIT OR Apache-2.0
