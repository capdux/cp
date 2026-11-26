# @cap/mysql-dsn

Zero-heap MySQL / MariaDB / TiDB connection string parser (`mysql://`, `mysqls://`, `mariadb://`, and Go-MySQL `user:pass@tcp(host:3306)/db`) with password redaction for CP.
Compatible with npm `mysql2` and Rust `mysql`.

## API

- `MysqlConfig { tls, host, port, user, password, database, charset, valid }`
- `parse(input: string): MysqlConfig`
- `parseUri(uri: string): MysqlConfig`
- `parseGoDsn(dsn: string): MysqlConfig`
- `formatUri(cfg: MysqlConfig, redact_password: bool, out: []u8): usize`

## License

MIT
