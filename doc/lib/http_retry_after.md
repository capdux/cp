# `cap:http_retry_after` — RFC 9110 §10.2.3 `Retry-After` Header Parser & Formatter

Zero-allocation RFC 9110 §10.2.3 `Retry-After` parser supporting both delta-seconds (`"120"`) and IMF-fixdate (`"Wed, 21 Oct 2015 07:28:00 GMT"`), relative wait calculation, and clamping.

## Exports

- `RetryAfterKind` (`delay_seconds` | `http_date`)
- `RetryAfter` — `parse(header): RetryAfter?`, `waitSeconds(now_unix_sec): u64`, `waitSecondsCapped(now_unix_sec, max_wait_sec): u64`
- `formatRetryAfterSeconds(seconds, out): ([]const u8)?`
