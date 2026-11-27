[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# schema-utils

> 版本：`4.3.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap configuration option schema validator in pure CP

## 安装与导入

```bash
cap add schema-utils
```

```cp
import { schuStrEq } from "schema-utils";
```

## 核心导出 API

- `export fn schuStrEq(a: string, b: string): bool;`
- `export fn schuOk(): ValidationResult;`
- `export fn schuFail(code: u32, msg: string): ValidationResult;`
- `export fn validateStringField(value: string, present: bool, c: FieldConstraint): ValidationResult;`
- `export fn validateIntField(value: i64, present: bool, c: FieldConstraint): ValidationResult;`
- `export fn validateEnumField(value: string, allowed: &[]string): ValidationResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`schema-utils@4.3.0`)（https://github.com/webpack/schema-utils），许可证：MIT
