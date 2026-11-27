[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/db

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official unified database context and request-scoped automatic transaction plugin for cap:web

## 安装与导入

```bash
cap add @web/db
```

```cp
import { dbDefaultTxAction } from "@web/db";
```

## 核心导出 API

- `export enum DbDialect`
- `export fn dbDefaultTxAction(action: u8): bool;`
- `export fn dbRunTxAction(action: u8): bool;`
- `export fn dbStrEql(a: string, b: string): bool;`
- `export fn dbIsWriteMethod(m: AppMethod): bool;`
- `export fn dbTxStats(): DbTxStats;`
- `export fn dbResetStats();`
- `export fn dbBeforeHandle(ctx: &mut Context): bool;`
- `export fn dbAfterHandle(ctx: &mut Context, resp: &mut Response);`
- `export fn createDbPlugin(cfg: DbPluginConfig, action_fn: DbTxActionFn): Plugin;`
- `export fn registerDbWithHook(app: &mut App, cfg: DbPluginConfig, action_fn: DbTxActionFn): &mut App;`
- `export fn registerDb(app: &mut App, cfg: DbPluginConfig): &mut App;`
