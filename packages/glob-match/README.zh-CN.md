[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# glob-match

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation, anti-backtracking path glob and brace alternation matching engine for CP.

## 安装与导入

```bash
cap add glob-match
```

```cp
import { gbmFindClassClose } from "glob-match";
```

## 核心导出 API

- `export fn gbmFindClassClose(pattern: string, open_idx: usize): usize;`
- `export fn gbmMatchCharClass(pattern: string, open_idx: usize, close_idx: usize, ch: u8): bool;`
- `export fn gbmMatchCore(pattern: string, path: string, record_caps: bool, out_caps: []GlobSpan, out_count: &mut usize): bool;`
- `export fn gbmFindBracePair(pattern: string, open_out: &mut usize, close_out: &mut usize): bool;`
- `export fn gbmCopySlice(dst: []u8, pos: usize, src: string, s: usize, e: usize): usize;`
- `export fn gbmMatchWithBraces(pattern: string, path: string, depth: usize, steps: &mut usize, record_caps: bool, out_caps: []GlobSpan, out_count: &mut usize): bool;`
- `export fn globMatch(pattern: string, path: string): bool;`
- `export fn globMatchAny(patterns: &[]string, path: string): bool;`
- `export fn globMatchCaptures(pattern: string, path: string, out_caps: []GlobSpan): usize;`
- `export fn isStaticGlob(pattern: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`glob-match@1.0.0`)，许可证：MIT
