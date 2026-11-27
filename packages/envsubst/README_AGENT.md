# envsubst@1.0.0 (Agent Quickref)

> Zero-allocation POSIX / Docker / K8s environment variable template expansion engine (dotenv-expand / envsub / Rust shellexpand parity) for CP | License: MIT
> Import: `import { ... } from "envsubst";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum ExpandError`

## Functions
- `export fn isVarStart(c: u8): bool;`
- `export fn isVarCont(c: u8): bool;`
- `export fn expandImpl(tpl: string, env: EnvMap, out: []u8, start_out: usize, depth: u8): ExpandResult;`
- `export fn expandEnv(tpl: string, env: EnvMap, out: []u8): ExpandResult;`
- `export fn expandEnvStr(tpl: string, env: EnvMap, out: []u8): string;`
- `export fn hasEnvVars(tpl: string): bool;`
