[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# find-up

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap upward ancestor directory file/directory search walker with stopAt boundary and multi-name candidate generation (npm find-up equivalent, reuses npm-run-path)

## 安装与导入

```bash
cap add find-up
```

```cp
import { fupStrEq } from "find-up";
```

## 核心导出 API

- `export fn fupStrEq(a: string, b: string): bool;`
- `export fn fupTrimTrailingSlash(s: string): string;`
- `export fn fupPush(out: []u8, pos: usize, s: string): usize;`
- `export fn joinDirAndName(dir: string, name: string, out: []u8): usize;`
- `export fn findUpInList(opts: FindUpOptions, filename: string, existing_paths: []string, out: []u8): usize?;`
- `export fn findUpMultipleInList(opts: FindUpOptions, filenames: []string, existing_paths: []string, out: []u8): usize?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`find-up@1.0.0`)，许可证：MIT
