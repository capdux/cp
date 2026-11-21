# `cap:http_if_match` — RFC 9110 §13.1 Conditional Request Evaluator

Zero-allocation RFC 9110 §13.2.2 HTTP conditional request evaluator (`If-Match`, `If-Unmodified-Since`, `If-None-Match`, `If-Modified-Since`) with strong/weak ETag comparison.

## Exports

- `EntityTag` — `parse`, `strongEquals`, `weakEquals`.
- `etagListMatches(header, current_etag, strong_mode): bool` — Evaluates comma-separated ETag lists and `*`.
- `PreconditionResult` (`proceed` | `not_modified_304` | `precondition_failed_412`).
- `ConditionalRequest` & `evaluatePreconditions(req, current_etag, last_modified_sec): PreconditionResult`.
