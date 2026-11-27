[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# set-cookie-parser

Zero-allocation RFC 6265 `Set-Cookie` HTTP header parser and comma-joined header splitter in pure CP.

## API

- `parseSetCookie(header: string): ParsedCookie` — parses name, value (unquoting `"..."`), `Path`, `Domain`, `Expires`, `Max-Age`, `SameSite`, `Secure`, `HttpOnly`, and `Partitioned` attributes (case-insensitive).
- `splitCookiesCount(combined: string): usize` — counts cookies in a comma-combined `Set-Cookie` header without splitting on `Expires=Wed, ...` commas.
- `splitCookiesGet(combined: string, target_idx: usize): string?` — extracts the `target_idx`-th cookie string from a comma-combined header.

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`set-cookie-parser@2.7.1`)
- **Upstream Repository**: https://github.com/nfriedly/set-cookie-parser
- **License**: MIT (clean-room CP implementation)
