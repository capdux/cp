[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# cors

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cors` ([https://github.com/expressjs/cors](https://github.com/expressjs/cors))（原库名 `cors` → CP 包名 `cors`）

Clean-room MIT, zero-heap Fetch/W3C CORS policy evaluator and response header builder for CP — modeled on npm's `cors`.

## Dependencies
- `unicase` (`1.0.0`) — case-insensitive HTTP method/header comparison
- `wildmatch` (`1.0.0`) — wildcard origin matching (e.g., `https://*.example.com`)

## API
- `CorsOptions` (`permissive`, `restrictive`, `addOrigin`)
- `isOriginAllowed(opts, origin)` & `isMethodAllowed(allowed_methods, method)`
- `evaluateCors(opts, method, origin, ac_req_method, ac_req_headers): CorsDecision`
- `formatCorsHeaders(decision, out): usize`
