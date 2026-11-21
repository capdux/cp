# `cap:http_origin` — WHATWG Origin Parser, Comparator & Secure-Context Checker

Zero-allocation WHATWG URL Origin tuple parser, default-port normalizer, Same-Origin comparator, Secure Contexts trustworthiness checker, and canonical serializer.

## Exports

- `Origin` — `parse(raw): Origin?`, `sameOrigin(other): bool`, `isPotentiallyTrustworthy(): bool`, `format(out): ([]const u8)?`.
- `isSameOriginUrl(a, b): bool` — Direct same-origin check for two URL strings.
