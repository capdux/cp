[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# test-exclude

> 版本：`7.0.1` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap test and coverage file path glob filter engine

## 安装与导入

```bash
cap add test-exclude
```

```cp
import { texcHasPrefix } from "test-exclude";
```

## 核心导出 API

- `export fn texcHasPrefix(s: string, prefix: string): bool;`
- `export fn texcHasSuffix(s: string, suffix: string): bool;`
- `export fn texcContainsSegment(path: string, seg: string): bool;`
- `export fn texcMatchAt(pat: string, pi: usize, text: string, ti: usize): bool;`
- `export fn texcMatchRaw(pat: string, path: string): bool;`
- `export fn matchSimpleGlob(pattern: string, path: string): bool;`
- `export fn texcMatchesExt(path: string, ext: string): bool;`
- `export fn shouldInstrument(rel_path: string, cfg: ExcludeConfig): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`test-exclude@7.0.1`)（https://github.com/istanbuljs/test-exclude），许可证：ISC
