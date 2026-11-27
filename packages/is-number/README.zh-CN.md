[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# is-number

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap strict numeric string validator (integers, decimals, scientific, hex) matching npm is-number in pure CP

## 安装与导入

```bash
cap add is-number
```

```cp
import { isSpace } from "is-number";
```

## 核心导出 API

- `export fn isSpace(c: u8): bool;`
- `export fn isDigit(c: u8): bool;`
- `export fn isHexDigit(c: u8): bool;`
- `export fn isNumber(s: string): bool;`
- `export fn isIntegerStr(s: string): bool;`
- `export fn isFiniteF64(x: f64): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`is-number@1.0.0`)，许可证：MIT
