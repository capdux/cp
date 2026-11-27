[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# lilconfig

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure CP lightweight configuration explorer with JSON/JS defaults and custom loader registry (clean-room MIT implementation of npm lilconfig v3)

## 安装与导入

```bash
cap add lilconfig
```

```cp
import { lilToLower } from "lilconfig";
```

## 核心导出 API

- `export fn lilToLower(b: u8): u8;`
- `export fn lilEqIgnoreCase(a: string, b: string): bool;`
- `export fn lilEndsWithIgnoreCase(s: string, suffix: string): bool;`
- `export fn lilWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn lilBasename(filepath: string): string;`
- `export fn lilIsNoExt(filepath: string): bool;`
- `export fn formatLilSearchPlace(module_name: string, idx: usize, out: []u8): usize;`
- `export fn isSupportedByDefault(filepath: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`lilconfig@1.0.0`)，许可证：MIT
