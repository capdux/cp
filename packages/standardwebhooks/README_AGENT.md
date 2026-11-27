# standardwebhooks — AI Agent Quick Ref

- **Package**: `standardwebhooks` (`1.0.0`) | **Source**: npm & crates.io `standardwebhooks` (Stripe + GitHub + Standard Webhooks)
- **Import**: `import { StripeSig, MAX_V1_SIGS, parseStripeHeader, parseGitHubSig256, parseStandardWebhookSig, isWithinTolerance, constantTimeEq, verifyStripeExpected, formatStripeSignedPayload } from "standardwebhooks";`
- **Allocation**: Zero-heap; uses constant-time comparison (`cap:bin.ctEqual`).

## Constants & Core Signatures

```typescript
export const MAX_V1_SIGS: usize = 4;

export class StripeSig {
    timestamp: i64,
    v1_sigs: [4]string,
    v1_count: usize,
    valid: bool,
}

export fn parseNonNegI64(s: string): i64;
export fn parseStripeHeader(header: string): StripeSig;
export fn parseGitHubSig256(header: string): string;
export fn parseStandardWebhookSig(header: string, out_sigs: []string): usize;
export fn isWithinTolerance(timestamp_secs: i64, now_secs: i64, tolerance_secs: i64): bool;
export fn constantTimeEq(a: string, b: string): bool;
export fn verifyStripeExpected(header: string, expected_v1_hex: string, now_secs: i64, tolerance_secs: i64): bool;
export fn formatStripeSignedPayload(timestamp_secs: i64, payload: string, out: []u8): usize;
```

## Minimal Pattern

```typescript
import { parseStripeHeader, formatStripeSignedPayload, verifyStripeExpected } from "standardwebhooks";

const sig = parseStripeHeader(header);
if (sig.valid) {
    let buf: [2048]u8 = undefined;
    const n = formatStripeSignedPayload(sig.timestamp, raw_body, buf[0..]);
    // Compute HMAC-SHA256 hex of buf[0..n], then verify:
    const ok = verifyStripeExpected(header, expected_hex, now_secs, 300);
}
```

## Pitfalls

1. **HMAC computation is external**: This package handles header parsing, payload formatting, replay tolerance, and constant-time comparison; compute HMAC-SHA256 with `cap:crypto` or `hmac` first.
2. **GitHub empty return**: `parseGitHubSig256` returns `""` if the `sha256=` prefix is missing or hex length != 64 — check `sig.len == 64` (or `> 0`) before comparing.
3. **Key rotation**: Standard Webhooks (`parseStandardWebhookSig`) and Stripe (`StripeSig.v1_sigs`) can contain multiple `v1` signatures; iterate all returned entries.
4. **Buffer check**: `formatStripeSignedPayload` returns `0` if `out` is too small or `timestamp_secs < 0`.
