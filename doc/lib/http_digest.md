# `cap:http_digest` — RFC 9530 `Content-Digest` / `Repr-Digest` & `Want-Content-Digest`

Zero-allocation RFC 9530 HTTP Digest Fields parser, formatter, and `Want-Content-Digest` algorithm negotiator.

## Exports

- `DigestEntry` / `DigestIter` — Parses RFC 8941 byte-sequence dictionary members `alg=:base64:`.
- `findDigest(header, algorithm): ([]const u8)?` — Case-insensitive digest lookup.
- `formatDigest(algorithm, base64_value, out): ([]const u8)?` — Canonical lowercase `alg=:base64:` formatter.
- `WantDigestPref` / `WantDigestIter` — Parses `Want-Content-Digest` preference weights (`0..=10`).
- `negotiateDigestAlg(want_header, supported): usize?` — Picks highest-weight supported algorithm (rejects weight `0`).
