# @web/pressure@1.0.0 (Agent Quickref)

> Official overload protection, body limit (413), load shedding (503), and K8s /healthz /readyz probes for cap:web | License: MIT
> Import: `import { ... } from "@web/pressure";` | Full types: `src/index.d.cp`

## Functions
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
