// Source: Absorbed from /data/dev/bun/src/http/HTTPContext.cp | Synced: 2026-11-22T11:12:13Z (Exponential Backoff & RFC 9110 Retry-After)

export {
    JitterMode,
    BackoffConfig,
    BackoffState,
    RetryAfter,
    parseRetryAfter,
    isRetryableHttpStatus,
} from "./backoff.cp";
