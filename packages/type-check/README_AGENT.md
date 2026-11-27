# type-check@1.0.0 (Agent Quickref)

> Zero-allocation runtime type expression parser and validator supporting primitives, unions (A | B), Maybe T, arrays ([T]), tuples ((A, B)), and objects (clean-room MIT implementation of npm type-check, #241 / 195M wk) | License: MIT
> Import: `import { ... } from "type-check";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum DynValueKind`

## Functions
- `export fn typeCheck(type_expr: string, val: DynValue): bool;`
- `export fn isValidTypeExpr(type_expr: string): bool;`
