[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# busboy

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 7578 multipart/form-data boundary extractor, part iterator, and Content-Disposition field/filename parser.

## 安装与导入

```bash
cap add busboy
```

```cp
import { isWs } from "busboy";
```

## 核心导出 API

- `export fn isWs(c: u8): bool;`
- `export fn isBchar(c: u8): bool;`
- `export fn isValidBoundary(boundary: string): bool;`
- `export fn extractBoundary(content_type: string): string;`
- `export fn matchBoundaryAt(payload: string, pos: usize, boundary: string): bool;`
- `export fn extractDispositionParam(disp: string, key: string): string;`
- `export fn nextPart(payload: string, boundary: string, start: usize): MultipartPart;`
- `export fn countParts(payload: string, boundary: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`busboy@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`multer@1.0.0`)，许可证：MIT
