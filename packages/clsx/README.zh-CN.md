[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# clsx

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap conditional CSS class name builder with whitespace normalization, deduplication, and word-boundary checks (npm clsx/classnames parity).

## 安装与导入

```bash
cap add clsx
```

```cp
import { isWs } from "clsx";
```

## 核心导出 API

- `export fn isWs(c: u8): bool;`
- `export fn bytesEq(a: &[]u8, b: &[]u8): bool;`
- `export fn joinClasses(classes: &[]&[]u8, dst: []u8): i64;`
- `export fn joinCondClasses(items: &[]CondClass, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`clsx@1.0.0`)，许可证：MIT
