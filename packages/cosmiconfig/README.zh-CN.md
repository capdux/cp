[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cosmiconfig

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure CP configuration file search planner, candidate path builder, and loader detector (clean-room MIT implementation of npm cosmiconfig v9)

## 安装与导入

```bash
cap add cosmiconfig
```

```cp
import { cosmiToLower } from "cosmiconfig";
```

## 核心导出 API

- `export fn cosmiToLower(b: u8): u8;`
- `export fn cosmiStrEq(a: string, b: string): bool;`
- `export fn cosmiEqIgnoreCase(a: string, b: string): bool;`
- `export fn cosmiEndsWithIgnoreCase(s: string, suffix: string): bool;`
- `export fn cosmiWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn cosmiBasename(filepath: string): string;`
- `export fn cosmiTrimTrailingSlash(dir: string): string;`
- `export fn cosmiLastSlash(s: string): usize;`
- `export fn detectLoader(filepath: string): u8;`
- `export fn formatSearchPlace(module_name: string, place_idx: usize, out: []u8): usize;`
- `export fn buildCandidatePath(dir: string, place: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cosmiconfig@1.0.0`)，许可证：MIT
