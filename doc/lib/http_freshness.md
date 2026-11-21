# `cap:http_freshness` — RFC 9111 §4.2 HTTP Cache Freshness & Age Calculator

Zero-allocation RFC 9111 §4.2 HTTP cache freshness lifetime (`s-maxage` > `max-age` > `Expires - Date` > 10% `Last-Modified` heuristic), RFC 9111 §4.2.3 corrected age calculation, and RFC 5861 `stale-while-revalidate` / `stale-if-error` evaluation.

## Exports

- `CacheFreshness` (`fresh` | `stale_while_revalidate` | `stale`)
- `FreshnessInput` — `empty`, `freshnessLifetimeSec()`, `currentAgeSec(now)`, `evaluate(now)`, `remainingFreshSec(now)`, `canServeStaleOnError(now)`.
