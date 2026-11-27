[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# resolve-from

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Node.js/CP module specifier resolver from a base directory with relative, scoped, and ancestor node_modules/cap_modules probing (npm resolve-from equivalent)

## 安装与导入

```bash
cap add resolve-from
```

```cp
import { rfExists } from "resolve-from";
```

## 核心导出 API

- `export fn rfExists(path: string, files: []string): bool;`
- `export fn isRelativeSpecifier(spec: string): bool;`
- `export fn isScopedSpecifier(spec: string): bool;`
- `export fn rfProbeBase(base: string, files: []string, out: []u8): usize?;`
- `export fn rfResolveRelativeBase(from_dir: string, rel: string, out: []u8): usize;`
- `export fn resolveFrom(from_dir: string, module_id: string, existing_files: []string, out: []u8): usize?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`resolve-from@1.0.0`)，许可证：MIT
