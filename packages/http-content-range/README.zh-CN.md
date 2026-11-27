[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# http-content-range

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 9110 HTTP Content-Range header parser and formatter (clean-room MIT implementation of crates.io http-content-range)

## 安装与导入

```bash
cap add http-content-range
```

```cp
import { parseContentRange } from "http-content-range";
```

## 核心导出 API

- `export enum ContentRangeKind`
- `export fn parseContentRange(header: string): ContentRange;`
- `export fn formatContentRangeHeader(cr: ContentRange, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`http-content-range@1.0.0`)，许可证：MIT OR Apache-2.0
