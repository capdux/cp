[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/env

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official startup environment schema validation & App state injection plugin for cap:web (inspired by @fastify/env)

## 安装与导入

```bash
cap add @web/env
```

```cp
import { envStrEql } from "@web/env";
```

## 核心导出 API

- `export enum EnvKind`
- `export fn envStrEql(a: string, b: string): bool;`
- `export fn envParseI64(s: string): i64?;`
- `export fn envIsValidBool(s: string): bool;`
- `export fn envInEnum(val: string, list: string): bool;`
- `export fn envLookupRaw(entries: &[]EnvKV, key: string): string?;`
- `export fn registerEnv(app: &mut App, specs: &[]EnvVarSpec, entries: &[]EnvKV): EnvValidationResult;`
