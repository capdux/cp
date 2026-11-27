# ajv@1.0.0 (Agent Quickref)

> Zero-heap JSON Schema Draft-07 / 2020-12 validation engine for CP (clean-room MIT implementation of npm ajv v8). | License: MIT
> Import: `import { ... } from "ajv";` | Full types: `src/index.d.cp`
> Deps: `ipaddr.js`, `zod`

## Functions
- `export fn validateFormat(fmt: string, value: string): bool;`
- `export fn validateWithSchema(schema_json: string, data_json: string): AjvResult;`
