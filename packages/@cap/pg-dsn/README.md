# @cap/pg-dsn

Zero-heap PostgreSQL connection string parser and URI builder supporting both `postgresql://` / `postgres://` URIs and libpq `key=value` DSNs in pure CP.

## Features

- `parseUri(uri)`: parses `postgresql://` and `postgres://` with `user[:pass]@`, IPv4/IPv6 `[::1]:5432`, `/dbname`, and `?sslmode=...`
- `parseKeyValue(dsn)`: parses libpq `k=v` pairs including single-quoted values (`password='a b'`)
- `parse(input)`: auto-detects URI or key=value format
- `isValidSslMode(mode)`: validates `disable`, `allow`, `prefer`, `require`, `verify-ca`, `verify-full`
- `formatUri(cfg, include_password, out)`: builds `postgresql://...` with safe password redaction when `include_password == false`

## Usage

```cp
import { parse, formatUri } from "@cap/pg-dsn";

const cfg = parse("postgresql://admin:s3cr3t@db.local:5432/app?sslmode=require");
let safe: [128]u8 = undefined;
const n = formatUri(cfg, false, safe[0..128]);
// safe[0..n] => "postgresql://admin@db.local:5432/app?sslmode=require"
```
