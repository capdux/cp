[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# glob-to-regexp

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation POSIX/glob wildcard to ECMAScript/PCRE regular expression converter with extended & globstar modes in pure CP

## 安装与导入

```bash
cap add glob-to-regexp
```

```cp
import { pushByte } from "glob-to-regexp";
```

## 核心导出 API

- `export fn pushByte(dst: []u8, pos: usize, b: u8): i64;`
- `export fn pushSlice(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn isEscapeMeta(b: u8): bool;`
- `export fn globToRegex(glob: &[]u8, opts: GlobOptions, dst: []u8): i64;`
- `export fn globToRegexDefault(glob: &[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`glob-to-regexp@1.0.0`)，许可证：BSD-2-Clause
