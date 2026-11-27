[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @web/pressure

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Official overload protection, body limit (413), load shedding (503), and K8s /healthz /readyz probes for cap:web

## 安装与导入

```bash
cap add @web/pressure
```

```cp
import { pressureSetReady } from "@web/pressure";
```

## 核心导出 API

- `export fn pressureSetReady(ready: bool);`
- `export fn pressureSetInflightForTest(n: u32);`
- `export fn pressureShedCount(): u64;`
- `export fn pressureResetStats();`
- `export fn pressStrEql(a: string, b: string): bool;`
- `export fn pressureOnRequest(ctx: &mut Context): bool;`
- `export fn pressureOnAfter(ctx: &mut Context, resp: &mut Response);`
- `export fn handleHealthz(ctx: &mut Context): Response;`
- `export fn handleReadyz(ctx: &mut Context): Response;`
- `export fn createPressurePlugin(cfg: PressureConfig): Plugin;`
- `export fn registerPressure(app: &mut App, cfg: PressureConfig): &mut App;`
