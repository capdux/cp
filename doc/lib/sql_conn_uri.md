# `cap:sql_conn_uri`

纯 CP 零堆分配 SQL 连接串解析器（PostgreSQL / MySQL / SQLite）、SSLMode 策略枚举与 PostgreSQL `CommandTag` 解析器，吸收自 `/data/dev/bun/src/sql/postgres/{CommandTag,SSLMode}.cp` 与 `sql/mysql/SSLMode.cp`。

## 导出符号

- `SqlDialect`：`postgres | mysql | sqlite` + `defaultPort()`
- `SslMode`：`disable | allow | prefer | require | verify_ca | verify_full` + `parse()`、`requiresTls()`、`verifiesCert()`、`verifiesHostname()`
- `PgCommandKind` / `PgCommandTag`：`parse("INSERT 0 42")`、`parse("UPDATE 7")`、`parse("CREATE TABLE")` 等
- `SqlConnUri`：`parse("postgres://user:pass@[::1]:5433/app?sslmode=verify-full&application_name=cp")`
