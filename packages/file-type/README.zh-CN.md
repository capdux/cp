[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# file-type

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation binary file magic bytes signature sniffer (25+ image, archive, document, executable, audio, video, and database formats) for CP.

## 安装与导入

```bash
cap add file-type
```

```cp
import { detect } from "file-type";
```

## 核心导出 API

- `export fn detect(data: &[]u8): FileType;`
- `export fn isImage(data: &[]u8): bool;`
- `export fn isArchive(data: &[]u8): bool;`
- `export fn isExecutable(data: &[]u8): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`file-type@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`infer@1.0.0`)，许可证：MIT
