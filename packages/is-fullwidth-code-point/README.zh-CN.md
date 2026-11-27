[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# is-fullwidth-code-point

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation UAX #11 Unicode fullwidth code point checker (clean-room MIT implementation of npm is-fullwidth-code-point v5)

## 安装与导入

```bash
cap add is-fullwidth-code-point
```

```cp
import { isFullwidthCodePoint } from "is-fullwidth-code-point";
```

## 核心导出 API

- `export fn isFullwidthCodePoint(cp: u32): bool;`
- `export fn codePointWidth(cp: u32): usize;`
- `export fn countFullwidthChars(s: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`is-fullwidth-code-point@1.0.0`)，许可证：MIT
