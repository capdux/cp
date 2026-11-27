[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/flags

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official deterministic FNV-1a Feature Flags & percentage rollout plugin for cap:web

## 安装与导入

```bash
cap add @web/flags
```

```cp
import { flgStrEql } from "@web/flags";
```

## 核心导出 API

- `export fn flgStrEql(a: string, b: string): bool;`
- `export fn flgInList(list: string, target: string): bool;`
- `export fn flagBucket(flag_name: string, user_key: string): u8;`
- `export fn flagReset();`
- `export fn flagDefine(name: string, enabled: bool, rollout_pct: u8, allow_users: string): bool;`
- `export fn flagEval(flag_name: string, user_key: string): bool;`
- `export fn flgWriteRaw(dst: []u8, pos: &mut usize, s: string): bool;`
- `export fn flgWriteU8(dst: []u8, pos: &mut usize, v: u8): bool;`
- `export fn handleFlagsStatus(ctx: &mut Context): Response;`
- `export fn createFlagsPlugin(cfg: FlagsConfig): Plugin;`
- `export fn registerFlags(app: &mut App, cfg: FlagsConfig): &mut App;`
