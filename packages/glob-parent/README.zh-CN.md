[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# glob-parent

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation glob static parent directory extractor, glob detector, and escaper (glob-parent + is-glob parity) for CP

## 安装与导入

```bash
cap add glob-parent
```

```cp
import { isGlob } from "glob-parent";
```

## 核心导出 API

- `export fn isGlob(pattern: string): bool;`
- `export fn globParent(pattern: string): string;`
- `export fn splitGlobBase(pattern: string): GlobSplit;`
- `export fn escapeGlob(raw: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`glob-parent@1.0.0`)，许可证：MIT
