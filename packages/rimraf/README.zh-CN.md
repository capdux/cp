[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# rimraf

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation recursive deletion safety guard, root-path validator, and bottom-up delete planner for CP, reusing fast-glob.

## 安装与导入

```bash
cap add rimraf
```

```cp
import { rmfIsAsciiAlpha } from "rimraf";
```

## 核心导出 API

- `export fn rmfIsAsciiAlpha(b: u8): bool;`
- `export fn rmfIsSep(b: u8, is_windows: bool): bool;`
- `export fn rmfHasNul(s: string): bool;`
- `export fn rmfPathDepth(path: string, is_windows: bool): usize;`
- `export fn isRootPath(path: string, is_windows: bool): bool;`
- `export fn validateRimrafTarget(target: string, opts: RimrafOptions): bool;`
- `export fn rmfSortByDepthDesc(paths: []string, count: usize, is_windows: bool);`
- `export fn planBottomUpDelete(entries: []string, opts: RimrafOptions): RimrafPlan;`
- `export fn planFilteredDelete(entries: []string, pattern: string, opts: RimrafOptions): RimrafPlan;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`rimraf@1.0.0`)，许可证：ISC
