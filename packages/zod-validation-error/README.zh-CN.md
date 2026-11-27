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
- `export fn defaultMessageBuilderOptions(): MessageBuilderOptions;`
- `export fn zverWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn zverWriteByte(out: []u8, pos: usize, ch: u8): usize;`
- `export fn zverIsIdentChar(ch: u8): bool;`
- `export fn zverIsDigitsOnly(s: string): bool;`
- `export fn zverIsSimpleIdentifier(s: string): bool;`
- `export fn formatIssuePath(segments: &[]string, out: []u8): usize;`
- `export fn formatValidationIssue(issue: ValidationIssue, include_path: bool, include_code: bool, out: []u8): usize;`
- `export fn formatValidationIssues(issues: &[]ValidationIssue, opts: FormatErrorOptions, out: []u8): usize;`
- `export fn createMessage(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): usize;`
- `export fn fromZodErrorInto(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): ValidationError;`
- `export fn fromZodError(issues: &[]ValidationIssue, opts: MessageBuilderOptions): ValidationError;`
- `export fn fromError(issues: &[]ValidationIssue, opts: MessageBuilderOptions): ValidationError;`
- `export fn fromErrorMessage(msg: string, opts: MessageBuilderOptions): ValidationError;`
- `export fn isValidationError(error_name: string): bool;`
- `export fn isValidationErrorLike(error_name: string, has_details: bool): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`zod-validation-error@3.4.0`)（https://github.com/causaly/zod-validation-error），许可证：MIT
