[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# strip-json-comments

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation JSONC comment and trailing-comma stripper with line/column-preserving whitespace mode (npm strip-json-comments parity) for CP

## 安装与导入

```bash
cap add strip-json-comments
```

```cp
import { stripComments } from "strip-json-comments";
```

## 核心导出 API

- `export fn stripComments(src: &[]u8, dst: []u8, opts: StripOptions): i64;`
- `export fn stripJsonc(src: &[]u8, dst: []u8): i64;`
- `export fn stripCompact(src: &[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`strip-json-comments@1.0.0`)，许可证：MIT
