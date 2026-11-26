# @cap/cors-match

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tower-http` ([https://github.com/tower-rs/tower-http](https://github.com/tower-rs/tower-http))（原库名 `tower-http` → CP 包名 `@cap/cors-match`）

Zero-heap CORS origin matcher, wildcard subdomain check, and preflight method/header validator for CP.
Compatible with npm `cors` and Rust `tower-http::cors`.

## API

- `isValidOrigin(origin: string): bool`
- `matchOrigin(origin: string, pattern: string, allow_credentials: bool): bool`
- `matchOriginList(origin: string, patterns: []const string, allow_credentials: bool): bool`
- `isMethodAllowed(method: string, allowed_methods: string): bool`
- `areHeadersAllowed(req_headers: string, allowed_headers: string): bool`
- `isSimpleMethod(method: string): bool`

## License

MIT
