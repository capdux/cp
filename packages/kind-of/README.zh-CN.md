[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# kind-of

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Fast zero-heap value & JSON/literal type classifier (clean-room CP implementation of npm kind-of)

## 安装与导入

```bash
cap add kind-of
```

```cp
import { koIsSpace } from "kind-of";
```

## 核心导出 API

- `export fn koIsSpace(c: u8): bool;`
- `export fn koIsDigit(c: u8): bool;`
- `export fn koEq(s: string, start: usize, end: usize, lit: string): bool;`
- `export fn koStartsWith(s: string, start: usize, end: usize, prefix: string): bool;`
- `export fn koIsNumber(s: string, start: usize, end: usize): bool;`
- `export fn koIsBigInt(s: string, start: usize, end: usize): bool;`
- `export fn koIsIsoDate(s: string, start: usize, end: usize): bool;`
- `export fn kindOf(raw: string): u8;`
- `export fn kindName(kind: u8): string;`
- `export fn kindOfName(raw: string): string;`
- `export fn isPrimitive(raw: string): bool;`
- `export fn isPlainObject(raw: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`kind-of@1.0.0`)，许可证：MIT
