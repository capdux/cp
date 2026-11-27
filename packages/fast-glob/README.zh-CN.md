[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fast-glob

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure CP fast-glob pattern set, dynamic pattern detector, and path escaper (clean-room MIT implementation of npm fast-glob v3)

## 安装与导入

```bash
cap add fast-glob
```

```cp
import { fglToLower } from "fast-glob";
```

## 核心导出 API

- `export fn fglToLower(b: u8): u8;`
- `export fn fglIsGlobMeta(b: u8): bool;`
- `export fn fglNormalizeRelPath(raw: string): string;`
- `export fn fglPathDepth(path: string): usize;`
- `export fn fglHasDotSegment(path: string): bool;`
- `export fn fglLowerInto(src: string, dst: []u8): usize;`
- `export fn fglMatchOne(pattern: string, path: string, case_sensitive: bool): bool;`
- `export fn isDynamicPattern(pattern: string): bool;`
- `export fn escapePath(path: string, out: []u8): usize;`
- `export fn convertPathToPattern(path: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`fast-glob@1.0.0`)，许可证：MIT
