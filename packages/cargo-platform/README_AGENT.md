# cargo-platform@1.0.0 (Agent Quickref)

> Zero-allocation LLVM/Rust target triple parser and Cargo cfg(...) expression evaluator for CP | License: MIT
> Import: `import { ... } from "cargo-platform";` | Full types: `src/index.d.cp`

## Functions
- `export fn invalidTriple(): TargetTriple;`
- `export fn parseTargetTriple(triple: string): TargetTriple;`
- `export fn isIdentStart(c: u8): bool;`
- `export fn isIdentCont(c: u8): bool;`
- `export fn skipWs(p: &mut CfgParser, src: string);`
- `export fn u32MatchesDecimal(n: u32, s: string): bool;`
- `export fn evalKeyValue(key: string, val: string, target: TargetEnv): bool;`
- `export fn evalFlag(flag: string, target: TargetEnv): bool;`
- `export fn parsePredicate(p: &mut CfgParser, src: string, target: TargetEnv): bool;`
- `export fn evalCfgExpr(expr: string, target: TargetEnv): CfgEvalResult;`
