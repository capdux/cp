[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# locate-path

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation path locator over candidate lists with entry-type, symlink, and ordering controls for CP.

## 安装与导入

```bash
cap add locate-path
```

```cp
import { lcpStrEq } from "locate-path";
```

## 核心导出 API

- `export fn lcpStrEq(a: string, b: string): bool;`
- `export fn lcpTrimTrailingSlash(s: string): string;`
- `export fn lcpCopyStr(dst: []u8, src: string): usize;`
- `export fn isAbsolutePath(p: string): bool;`
- `export fn lcpResolveCandidate(cwd: string, cand: string, out: []u8): usize;`
- `export fn lcpEntryMatches(entry: FsEntry, target: string, opts: LocateOptions): bool;`
- `export fn locatePathInEntries(candidates: []string, entries: []FsEntry, opts: LocateOptions, out: []u8): usize?;`
- `export fn locateAllPathsInEntries(candidates: []string, entries: []FsEntry, opts: LocateOptions, matched_indices: []usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`locate-path@1.0.0`)，许可证：MIT
