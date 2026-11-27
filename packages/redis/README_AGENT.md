# redis@1.0.0 (Agent Quickref)

> Zero-allocation Redis / Valkey / Sentinel connection URL parser (redis://, rediss://, unix://) and password-redacting formatter for CP. | License: MIT
> Import: `import { ... } from "redis";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseRedisUrl(url: string): RedisConfig;`
- `export fn formatRedisUrl(cfg: RedisConfig, redact_password: bool, out: []u8): usize;`
