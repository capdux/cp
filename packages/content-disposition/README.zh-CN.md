[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# content-disposition

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 6266 / RFC 5987 HTTP Content-Disposition header parser, formatter, filename* decoder, and filename sanitizer for CP

## 安装与导入

```bash
cap add content-disposition
```

```cp
import { parseContentDisposition } from "content-disposition";
```

## 核心导出 API

- `export fn parseContentDisposition(header: string): ContentDisposition;`
- `export fn sanitizeFilename(raw: string, out: []u8): usize;`
- `export fn decodeFilenameStar(raw_star: string, out: []u8): usize;`
- `export fn formatContentDisposition(disp_type: string, filename: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`content-disposition@1.0.0`)，许可证：MIT
