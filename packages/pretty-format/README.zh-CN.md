[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pretty-format

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure CP structured value pretty-printer with indentation, compact mode, and max-depth protection (clean-room MIT implementation of npm pretty-format)

## 安装与导入

```bash
cap add pretty-format
```

```cp
import { formatStringLiteral } from "pretty-format";
```

## 核心导出 API

- `export fn formatStringLiteral(s: string, escape: bool, out: []u8): usize;`
- `export fn formatPrimitiveInt(v: i64, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`pretty-format@1.0.0`)，许可证：MIT
