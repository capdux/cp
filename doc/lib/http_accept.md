# `cap:http_accept` — RFC 9110 Content Negotiation

Zero-allocation RFC 9110 §12.5 HTTP content negotiation library for `Accept`, `Accept-Encoding`, and `Accept-Language` headers.

## Exports

- `parseQValue(raw: []const u8): u16?` — Strict RFC 9110 §12.4.2 quality value parser returning integer millis in `[0, 1000]`.
- `MediaRange` — Parsed `type/subtype` with `q_millis: u16`, `specificity(): u8`, and `matches(offered: []const u8): bool`.
- `AcceptIter` — Zero-copy iterator over `Accept` header media ranges (quoted-string aware).
- `EncodingPref` / `AcceptEncodingIter` — Zero-copy iterator over `Accept-Encoding` header entries.
- `LangPref` / `AcceptLanguageIter` — Zero-copy iterator over `Accept-Language` header entries.
- `negotiateMediaType(accept_header: []const u8, offered: []const []const u8): usize?` — Selects best offered media type by q-value and specificity, respecting `q=0` exclusions.
- `negotiateEncoding(accept_encoding: []const u8, offered: []const []const u8): usize?` — Selects best offered content-coding (`br`, `gzip`, `zstd`, `identity`, `*`).
- `negotiateLanguage(accept_language: []const u8, offered: []const []const u8): usize?` — Selects best offered BCP-47 language tag (exact > subtag prefix > `*`).

## Example

```cp
import { negotiateMediaType, negotiateEncoding, negotiateLanguage } from "cap:http_accept";

const offered_types = [_][]const u8{ "text/html", "application/json" };
const mt = negotiateMediaType("text/html;q=0.8, application/json;q=0.9", offered_types[0..]);
// mt == 1 ("application/json")
```
