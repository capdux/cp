# `cap:pg_extended_query` — PostgreSQL v3 扩展查询协议前端消息编码器

源自 Bun `sql/postgres/protocol/{Parse,Describe,Execute,Close,PortalOrPreparedStatement}.cp`，提供带 `~init()` 确定性 RAII 释放的流水线式 PostgreSQL v3 扩展查询前端编码器（`Parse`/`Bind`/`Describe`/`Execute`/`Close`/`Sync`/`Flush`/`Query`/`Terminate`）。

## 导出符号

- `PgExtendedQueryError`、`TargetKind`（`statement` / `portal`）、`ParamFormat`（`text` / `binary`）、`BindParam`
- `ExtendedQueryBuilder`（含 `~init()` RAII）: `init` / `bytes` / `len` / `reset` / `writeParse` / `writeBind` / `writeDescribe` / `writeExecute` / `writeClose` / `writeSync` / `writeFlush` / `writeSimpleQuery` / `writeTerminate`
