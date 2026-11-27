[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# multer

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure CP multipart/form-data boundary extractor and upload policy validator (clean-room MIT implementation of npm multer v2)

## 安装与导入

```bash
cap add multer
```

```cp
import { extractMultipartBoundary } from "multer";
```

## 核心导出 API

- `export fn extractMultipartBoundary(content_type: string): string?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`multer@1.0.0`)，许可证：MIT
