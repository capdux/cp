[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# minimatch

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Minimal matching utility converting glob expressions (clean-room CP implementation of npm minimatch)

## 安装与导入

```bash
cap add minimatch
```

```cp
import { miniToPico } from "minimatch";
```

## 核心导出 API

- `export fn miniToPico(opts: MinimatchOptions): PicoOptions;`
- `export fn isCommentPattern(pattern: string, opts: MinimatchOptions): bool;`
- `export fn isNegatedPattern(pattern: string, opts: MinimatchOptions): bool;`
- `export fn minimatchWithOptions(path: string, pattern: string, opts: MinimatchOptions): bool;`
- `export fn minimatch(path: string, pattern: string): bool;`
- `export fn minimatchList(paths: &[]string, pattern: string, opts: MinimatchOptions, out_indices: []usize): usize;`
- `export fn minimatchFilterAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize;`
- `export fn minimatchRejectAll(paths: &[]string, patterns: &[]string, opts: MinimatchOptions, out_indices: []usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`minimatch@1.0.0`)，许可证：ISC
