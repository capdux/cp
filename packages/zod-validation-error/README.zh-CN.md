[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# zod-validation-error

> 版本：`3.4.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap human-readable validation error aggregator and formatter (npm zod-validation-error equivalent)

## 安装与导入

```bash
cap add zod-validation-error
```

```cp
import { defaultFormatErrorOptions } from "zod-validation-error";
```

## 核心导出 API

- `export fn defaultFormatErrorOptions(): FormatErrorOptions;`
- `export fn zverWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn zverWriteByte(out: []u8, pos: usize, ch: u8): usize;`
- `export fn formatValidationIssues(issues: &[]ValidationIssue, opts: FormatErrorOptions, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`zod-validation-error@3.4.0`)（https://github.com/causaly/zod-validation-error），许可证：MIT
