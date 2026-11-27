# schema-utils@4.3.0 (Agent Quickref)

> Zero-heap configuration option schema validator in pure CP | License: MIT
> Import: `import { ... } from "schema-utils";` | Full types: `src/index.d.cp`

## Functions
- `export fn schuStrEq(a: string, b: string): bool;`
- `export fn schuOk(): ValidationResult;`
- `export fn schuFail(code: u32, msg: string): ValidationResult;`
- `export fn validateStringField(value: string, present: bool, c: FieldConstraint): ValidationResult;`
- `export fn validateIntField(value: i64, present: bool, c: FieldConstraint): ValidationResult;`
- `export fn validateEnumField(value: string, allowed: &[]string): ValidationResult;`
