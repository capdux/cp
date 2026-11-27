[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# dedent

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap common leading indentation detector and dedent engine for multiline strings (compatible with npm strip-indent, dedent & Rust indoc)

## 安装与导入

```bash
cap add dedent
```

```cp
import { isHorizSpace } from "dedent";
```

## 核心导出 API

- `export fn isHorizSpace(c: u8): bool;`
- `export fn minIndent(input: string): usize;`
- `export fn stripIndent(input: string, out: []u8): usize;`
- `export fn indentLines(input: string, spaces: usize, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`dedent@1.0.0`)，许可证：MIT
