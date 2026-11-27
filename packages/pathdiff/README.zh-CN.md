[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pathdiff

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap relative path difference calculator computing path relative to a base directory (crates.io pathdiff equivalent)

## 安装与导入

```bash
cap add pathdiff
```

```cp
import { pdStrEq } from "pathdiff";
```

## 核心导出 API

- `export fn pdStrEq(a: string, b: string): bool;`
- `export fn pdPush(out: []u8, pos: usize, s: string): usize;`
- `export fn pdSplitNormalized(path: string): SegList;`
- `export fn diffPaths(target: string, base: string, out: []u8): usize?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`pathdiff@1.0.0`)，许可证：MIT OR Apache-2.0
