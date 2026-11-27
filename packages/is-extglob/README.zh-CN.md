[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# is-extglob

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Bash/Micromatch extglob pattern detector (@(...), !(...), *(...), +(...), ?(...)) for CP

## 安装与导入

```bash
cap add is-extglob
```

```cp
import { isExtglobPrefix } from "is-extglob";
```

## 核心导出 API

- `export fn isExtglobPrefix(ch: u8): bool;`
- `export fn matchExtglobEnd(str: string, start: usize): usize;`
- `export fn findExtglobStart(str: string): i64;`
- `export fn isExtglob(str: string): bool;`
- `export fn countExtglobs(str: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`is-extglob@1.0.0`)，许可证：MIT
