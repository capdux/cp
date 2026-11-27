[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pg-connection-string

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap PostgreSQL connection string parser and URI builder supporting both postgresql:// URIs and libpq key=value DSNs.

## 安装与导入

```bash
cap add pg-connection-string
```

```cp
import { makeInvalid } from "pg-connection-string";
```

## 核心导出 API

- `export fn makeInvalid(): PgConfig;`
- `export fn isWs(c: u8): bool;`
- `export fn parsePortStrict(s: string): i32;`
- `export fn isValidSslMode(mode: string): bool;`
- `export fn parseUri(uri: string): PgConfig;`
- `export fn parseKeyValue(dsn: string): PgConfig;`
- `export fn parse(input: string): PgConfig;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn appendU16(out: []u8, pos: usize, v: u16): i64;`
- `export fn formatUri(cfg: PgConfig, include_password: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`pg-connection-string@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`tokio-postgres@1.0.0`)，许可证：MIT
