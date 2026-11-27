[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/cron

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official zero-heap 5-field Cron scheduler plugin for cap:web (bitmap matcher, pause/resume, status endpoint)

## 安装与导入

```bash
cap add @web/cron
```

```cp
import { cronParseU8 } from "@web/cron";
```

## 核心导出 API

- `export fn cronParseU8(s: string): u8?;`
- `export fn cronParseField(field: string, min_val: u8, max_val: u8, is_weekday: bool): u64?;`
- `export fn cronWriteRaw(out_buf: []u8, pos: &mut usize, s: string): bool;`
- `export fn cronWriteU64(out_buf: []u8, pos: &mut usize, val: u64): bool;`
- `export fn parseCronExpr(expr: string): CronExpr?;`
- `export fn cronMatches(expr: &CronExpr, t: CronTime): bool;`
- `export fn cronGlobalScheduler(): &mut CronScheduler;`
- `export fn resetCronScheduler();`
- `export fn cronStatusHandler(ctx: &mut Context): Response;`
- `export fn createCronPlugin(cfg: CronConfig): Plugin;`
- `export fn registerCron(app: &mut App, cfg: CronConfig): &mut App;`
