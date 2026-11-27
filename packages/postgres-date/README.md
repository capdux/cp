<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:postgres-date` ([https://github.com/bendrucker/postgres-date](https://github.com/bendrucker/postgres-date)) | 许可：`MIT`

# `@cap/postgres-date`

零堆分配纯 CP PostgreSQL `DATE` / `TIMESTAMP` / `TIMESTAMPTZ` / `infinity` / `BC` 解析器与 UTC 毫秒时间戳转换库，对标 npm **#505** `postgres-date`（真实周下载 **1.12 亿/周**）。

## 核心 API

- `parsePgDate(input: string): PgDateTime`
- `toUnixMs(dt: PgDateTime): i64?`
