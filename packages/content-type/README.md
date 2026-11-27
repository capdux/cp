# @cap/content-type

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:content-type` ([https://github.com/jshttp/content-type](https://github.com/jshttp/content-type))

Clean-room MIT, zero-heap RFC 7231 HTTP `Content-Type` header parser and formatter for CP — modeled on npm `content-type` v1.0.

## Dependencies
- `@cap/unicase` (`1.0.0`) — reused for ASCII case-insensitive parameter name lookup (`eqAscii`).

## API

### Structs
- `ContentTypeParam { name: string, value: string }`
  - `ContentTypeParam.new(name, value): ContentTypeParam`
- `ContentType { valid: bool, media_type: string, params: [8]ContentTypeParam, param_count: usize }`
  - `getParam(name: string): string?` — case-insensitive parameter lookup.
  - `getParamUnescaped(name: string, out: []u8): usize` — case-insensitive lookup with RFC 7230 quoted-pair (`\"`, `\\`) unescaping into caller buffer `out`.
  - `charset(): string?` — shorthand for `getParam("charset")`.
  - `boundary(): string?` — shorthand for `getParam("boundary")`.

### Functions
- `parseContentType(header: string): ContentType`
  - Strictly parses `type/subtype; key=val; key2="quoted \"val\""` per RFC 7231.
  - Rejects invalid tokens, malformed/trailing semicolons, missing values, control characters, and unclosed quotes.
- `formatContentType(media_type: string, params: []const ContentTypeParam, out: []u8): usize`
  - Formats a media type and parameter slice into `out`, automatically quoting and backslash-escaping values containing spaces or special characters. Returns `0` if `media_type` or any parameter is invalid.
- `isValidMediaType(s: string): bool`
  - Validates an RFC 7231 `type/subtype` token pair.
- `unescapeQuotedString(raw: string, out: []u8): usize`
  - Unescapes backslash-escaped quoted-string content into `out`.
