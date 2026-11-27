[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/circuit-breaker

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official Envoy / Tower three-state circuit breaker (Closed / Open / HalfOpen) plugin for cap:web

## 安装与导入

```bash
cap add @web/circuit-breaker
```

```cp
import { cbNowSec } from "@web/circuit-breaker";
```

## 核心导出 API

- `export enum CircuitState`
- `export fn cbNowSec(): u64;`
- `export fn cbState(): CircuitState;`
- `export fn cbFailureCount(): u32;`
- `export fn cbSetTestClock(now_sec: u64);`
- `export fn cbReset();`
- `export fn cbBeforeHandle(ctx: &mut Context): bool;`
- `export fn cbAfterHandle(ctx: &mut Context, resp: &mut Response);`
- `export fn cbCreatePlugin(cfg: CircuitBreakerConfig): Plugin;`
- `export fn registerCircuitBreaker(app: &mut App, cfg: CircuitBreakerConfig): &mut App;`
