[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# relative-path

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Portable platform-independent relative path manipulation, normalization, joining, and resolution (zero-heap pure CP)

## 安装与导入

```bash
cap add relative-path
```

```cp
import { rpIsSep } from "relative-path";
```

## 核心导出 API

- `export fn rpIsSep(b: u8): bool;`
- `export fn normalizeRelPath(path: string): RelPath;`
- `export fn joinRelPath(base: string, rel: string): RelPath;`
- `export fn relativeTo(from_path: string, to_path: string): RelPath;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`relative-path@1.0.0`)，许可证：MIT OR Apache-2.0
