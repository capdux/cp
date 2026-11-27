# schema-utils@4.3.0 (Agent Quickref)

> Zero-heap configuration option schema validator in pure CP | License: MIT
> Import: `import { ... } from "schema-utils";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum SchemaType`

## Functions
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
