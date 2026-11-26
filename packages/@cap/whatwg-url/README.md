# @cap/whatwg-url

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:url` ([https://github.com/servo/rust-url](https://github.com/servo/rust-url))（原库名 `url` → CP 包名 `@cap/whatwg-url`）

Pure CP, zero-allocation WHATWG URL Standard parser, normalizer, and relative URL resolver (clean-room MIT parity with Rust `url` v2).

## Dependencies

- `@cap/percent-encoding` — WHATWG `userinfoSet` / `pathSet` / `querySet` / `fragmentSet`, `percentEncode`, and `percentDecode`
- `@cap/idna-uts46` — UTS #46 `domainToAscii` for internationalized domain names (IDNA / Punycode `xn--...`)

## Features

- **Full WHATWG URL Parsing (`WhatwgUrl.parse`)**:
  - Lowercases scheme; recognizes special schemes (`http`, `https`, `ws`, `wss`, `ftp`, `file`)
  - Parses `user:pass@host:port` with `userinfoSet()` encoding
  - Supports IPv6 host literals (`[::1]:8080`) and IDNA domains (`münchen.de` -> `xn--mnchen-3ya.de`)
  - Automatically omits canonical default ports (`http:80`, `https:443`, `ws:80`, `wss:443`, `ftp:21`)
  - Normalizes dot segments (`.` / `..`) and percent-encodes paths, queries, and fragments
- **Relative URL Resolution (`WhatwgUrl.join`)**:
  - Supports absolute URLs, protocol-relative (`//host/path`), root-relative (`/x`), relative paths (`../y`), query-only (`?q=2`), and fragment-only (`#h`)
- **Serialization & Origin**:
  - `toStr(out: []u8): usize` and `origin(out: []u8): usize` (`scheme://host[:port]` or `"null"`)
- **Zero Heap Allocation**: Fixed stack buffers inside `WhatwgUrl`.

## API

- `defaultPortForScheme(scheme: string): i32`
- `struct WhatwgUrl`
  - `WhatwgUrl.parse(input: string): WhatwgUrl`
  - `WhatwgUrl.join(base: WhatwgUrl, rel: string): WhatwgUrl`
  - `toStr(out: []u8): usize`
  - `origin(out: []u8): usize`
  - `effectivePort(): i32`
  - `schemeStr()`, `usernameStr()`, `passwordStr()`, `hostStr()`, `pathStr()`, `queryStr()`, `fragmentStr()`

## License

MIT
