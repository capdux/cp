[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# error-ex

> 版本：`1.3.2` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap structured error and context formatter with template interpolation

## 安装与导入

```bash
cap add error-ex
```

```cp
import { erexWriteByte } from "error-ex";
```

## 核心导出 API

- `export fn erexWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn erexWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn erexWriteI32(out: []u8, pos: usize, val: i32): usize;`
- `export fn formatStructuredError(e: StructuredError, fields: &[]ErrorField, out: []u8): usize;`
- `export fn erexMatchesPlaceholder(tmpl: string, start: usize, key: string): bool;`
- `export fn formatErrorTemplate(tmpl: string, key: string, val: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`error-ex@1.3.2`)（https://github.com/Qix-/node-error-ex），许可证：MIT
