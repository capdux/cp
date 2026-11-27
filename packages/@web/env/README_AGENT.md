# @web/env@1.0.0 (Agent Quickref)

> Official startup environment schema validation & App state injection plugin for cap:web (inspired by @fastify/env) | License: MIT
> Import: `import { ... } from "@web/env";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum EnvKind`

## Functions
- `export fn envStrEql(a: string, b: string): bool;`
- `export fn envParseI64(s: string): i64?;`
- `export fn envIsValidBool(s: string): bool;`
- `export fn envInEnum(val: string, list: string): bool;`
- `export fn envLookupRaw(entries: &[]EnvKV, key: string): string?;`
- `export fn registerEnv(app: &mut App, specs: &[]EnvVarSpec, entries: &[]EnvKV): EnvValidationResult;`
