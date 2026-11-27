[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# is-glob

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation glob pattern detector reusing @cap/is-extglob for CP

## 安装与导入

```bash
cap add is-glob
```

```cp
import { hasClosedBracket } from "is-glob";
```

## 核心导出 API

- `export fn hasClosedBracket(str: string, open_idx: usize): bool;`
- `export fn hasClosedBrace(str: string, open_idx: usize): bool;`
- `export fn hasUnescapedWildcard(str: string): bool;`
- `export fn isGlob(str: string, strict: bool): bool;`
- `export fn isGlobDefault(str: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`is-glob@1.0.0`)，许可证：MIT
