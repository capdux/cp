[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# mysql2

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap MySQL / MariaDB / TiDB connection string parser (mysql:// URI & Go-style user:pass@tcp(host:port)/db) with password redaction for CP (compatible with npm mysql2 & Rust mysql)

## 安装与导入

```bash
cap add mysql2
```

```cp
import { eqBytes } from "mysql2";
```

## 核心导出 API

- `export fn eqBytes(a: string, b: string): bool;`
- `export fn startsWith(s: string, prefix: string): bool;`
- `export fn parsePort(s: string): i32;`
- `export fn indexOf(s: string, ch: u8): i64;`
- `export fn lastIndexOf(s: string, ch: u8): i64;`
- `export fn applyQueryParams(query: string, cfg: &mut MysqlConfig);`
- `export fn splitHostPort(hp: string, out_host: &mut string, out_port: &mut u16): bool;`
- `export fn parseUri(uri: string): MysqlConfig;`
- `export fn parseGoDsn(dsn: string): MysqlConfig;`
- `export fn parse(input: string): MysqlConfig;`
- `export fn appendStr(dst: []u8, pos: usize, src: string): usize;`
- `export fn appendU16(dst: []u8, pos: usize, val: u16): usize;`
- `export fn formatUri(cfg: MysqlConfig, redact_password: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`mysql2@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`mysql@1.0.0`)，许可证：MIT
