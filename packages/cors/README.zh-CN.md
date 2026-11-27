[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cors

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Fetch/W3C CORS origin evaluator and preflight/actual response header builder (clean-room port of npm cors)

## 安装与导入

```bash
cap add cors
```

```cp
import { corsPush } from "cors";
```

## 核心导出 API

- `export fn corsPush(out: []u8, pos: usize, s: string): usize;`
- `export fn corsPushU32(out: []u8, pos: usize, v: u32): usize;`
- `export fn corsIsWs(b: u8): bool;`
- `export fn corsTrim(s: string): string;`
- `export fn isOriginAllowed(opts: CorsOptions, request_origin: string): bool;`
- `export fn isMethodAllowed(allowed_methods: string, method: string): bool;`
- `export fn evaluateCors(opts: CorsOptions, method: string, origin: string, ac_req_method: string, ac_req_headers: string): CorsDecision;`
- `export fn formatCorsHeaders(d: CorsDecision, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cors@1.0.0`)，许可证：MIT
