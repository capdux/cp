[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# schema-utils

> 版本：`4.3.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap configuration option schema validator in pure CP

## 安装与导入

```bash
cap add schema-utils
```

```cp
import { schemaTypeName } from "schema-utils";
```

## 核心导出 API

- `export enum SchemaType`
- `export fn schemaTypeName(t: SchemaType): string;`
- `export fn schuStrEq(a: string, b: string): bool;`
- `export fn schuWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn schuOk(): ValidationResult;`
- `export fn schuFail(code: u32, msg: string): ValidationResult;`
- `export fn validateStringField(value: string, present: bool, c: FieldConstraint): ValidationResult;`
- `export fn validateIntField(value: i64, present: bool, c: FieldConstraint): ValidationResult;`
- `export fn validateEnumField(value: string, allowed: &[]string): ValidationResult;`
- `export fn schuMatchClass(pat: string, start: usize, end: usize, c: u8): bool;`
- `export fn schuAtomLen(pat: string, pi: usize): usize;`
- `export fn schuMatchAtom(pat: string, pi: usize, atom_len: usize, c: u8): bool;`
- `export fn schuMatchHere(pat: string, pi: usize, text: string, ti: usize): bool;`
- `export fn matchPattern(pattern: string, text: string): bool;`
- `export fn validatePatternField(value: string, pattern: string): ValidationResult;`
- `export fn schuTypeMatches(expected: SchemaType, actual: SchemaType): bool;`
- `export fn schuPushError(report: &mut ValidationReport, err: ValidationError);`
- `export fn validate(schema: ObjectSchema, values: &[]SchemaValue): ValidationReport;`
- `export fn formatValidationError(schema_name: string, err: ValidationError, out: []u8): usize;`
- `export fn formatValidationReport(schema_name: string, report: ValidationReport, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`schema-utils@4.3.0`)（https://github.com/webpack/schema-utils），许可证：MIT
