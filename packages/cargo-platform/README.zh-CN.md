[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cargo-platform

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation LLVM/Rust target triple parser and Cargo cfg(...) expression evaluator for CP

## 安装与导入

```bash
cap add cargo-platform
```

```cp
import { invalidTriple } from "cargo-platform";
```

## 核心导出 API

- `export fn invalidTriple(): TargetTriple;`
- `export fn parseTargetTriple(triple: string): TargetTriple;`
- `export fn isWs(c: u8): bool;`
- `export fn isIdentStart(c: u8): bool;`
- `export fn isIdentCont(c: u8): bool;`
- `export fn skipWs(p: &mut CfgParser, src: string);`
- `export fn strEq(a: string, b: string): bool;`
- `export fn u32MatchesDecimal(n: u32, s: string): bool;`
- `export fn evalKeyValue(key: string, val: string, target: TargetEnv): bool;`
- `export fn evalFlag(flag: string, target: TargetEnv): bool;`
- `export fn parsePredicate(p: &mut CfgParser, src: string, target: TargetEnv): bool;`
- `export fn evalCfgExpr(expr: string, target: TargetEnv): CfgEvalResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`cargo-platform@1.0.0`)，许可证：MIT
