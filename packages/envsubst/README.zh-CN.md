[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# envsubst

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation POSIX / Docker / K8s environment variable template expansion engine (dotenv-expand / envsub / Rust shellexpand parity) for CP

## 安装与导入

```bash
cap add envsubst
```

```cp
import { isVarStart } from "envsubst";
```

## 核心导出 API

- `export enum ExpandError`
- `export fn isVarStart(c: u8): bool;`
- `export fn isVarCont(c: u8): bool;`
- `export fn expandImpl(tpl: string, env: EnvMap, out: []u8, start_out: usize, depth: u8): ExpandResult;`
- `export fn expandEnv(tpl: string, env: EnvMap, out: []u8): ExpandResult;`
- `export fn expandEnvStr(tpl: string, env: EnvMap, out: []u8): string;`
- `export fn hasEnvVars(tpl: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`envsubst@1.0.0`)（https://github.com/droundy/envsubst），许可证：BSD-2-Clause
