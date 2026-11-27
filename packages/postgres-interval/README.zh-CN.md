[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# postgres-interval

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation PostgreSQL INTERVAL parser and ISO-8601 / PostgreSQL bidirectional formatter in pure CP

## 安装与导入

```bash
cap add postgres-interval
```

```cp
import { pgivInvalid } from "postgres-interval";
```

## 核心导出 API

- `export fn pgivInvalid(): PgInterval;`
- `export fn pgivZero(): PgInterval;`
- `export fn pgivIsSpace(c: u8): bool;`
- `export fn pgivIsDigit(c: u8): bool;`
- `export fn pgivIsAlpha(c: u8): bool;`
- `export fn pgivToLower(c: u8): u8;`
- `export fn pgivEqIgnoreCase(a: string, b: string): bool;`
- `export fn pgivSkipSpace(s: string, start: usize): usize;`
- `export fn pgivParseUint(s: string, start: usize): PgivIntRes;`
- `export fn pgivParseFracMs(s: string, start: usize): PgivFracRes;`
- `export fn pgivAbsI32ToU64(v: i32): u64;`
- `export fn pgivParseIso(s: string, start: usize, end: usize): PgInterval;`
- `export fn parsePgInterval(input: string): PgInterval;`
- `export fn toIsoDuration(iv: PgInterval, out: []u8): usize;`
- `export fn toPostgresString(iv: PgInterval, out: []u8): usize;`
- `export fn totalSecondsApprox(iv: PgInterval): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`postgres-interval@1.0.0`)，许可证：MIT
