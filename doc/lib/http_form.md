# `cap:http_form` — WHATWG `application/x-www-form-urlencoded` Parser & Builder

Zero-allocation WHATWG `application/x-www-form-urlencoded` encoder, decoder (`+` <-> space), streaming `FormIter`, key lookup, and builder.

## Exports

- `encodeFormComponent(raw, out): ([]const u8)?` — Encodes space as `+` and non-safe bytes as uppercase `%XX`.
- `decodeFormComponent(encoded, out): ([]const u8)?` — Decodes `+` as space and `%XX` bytes (preserving malformed `%` verbatim).
- `FormPairRaw` / `FormIter` — Zero-copy `&`-delimited pair iterator (strips optional leading `?`, skips empty segments).
- `getFormParam(body, target_key, out): ([]const u8)?`
- `appendFormParam(out, pos: *usize, key, val): bool`
