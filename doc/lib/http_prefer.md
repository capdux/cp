# `cap:http_prefer` — RFC 7240 `Prefer` & `Preference-Applied` Header

Zero-allocation RFC 7240 HTTP `Prefer` header parser, streaming iterator, and `Preference-Applied` formatter.

## Exports

- `Preference` — Single parsed preference (`name`, `value: ([]const u8)?`, `params`).
- `PreferIter` — Quoted-string-aware comma-separated iterator (`init`, `next`).
- `ReturnPref` (`none` | `representation` | `minimal`) & `HandlingPref` (`none` | `strict` | `lenient`).
- `PreferHeader` — `parse(header)` (first-occurrence-wins per RFC 7240 §2) and `formatApplied(out): ([]const u8)?`.
