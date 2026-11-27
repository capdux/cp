[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# standardwebhooks

Pure CP, zero-allocation **Standard Webhooks**, **Stripe (`Stripe-Signature`)**, and **GitHub (`X-Hub-Signature-256`)** signature header parser, replay-window timestamp guard, signed-payload formatter, and constant-time verifier.

## Provenance & Zero-Cost Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | [npm](https://www.npmjs.com/package/standardwebhooks) & [crates.io](https://crates.io/crates/standardwebhooks) |
| **Upstream Package** | `standardwebhooks` (v1 parity) |
| **Upstream Repo** | https://github.com/standard-webhooks/standard-webhooks |
| **CP Package** | `standardwebhooks` (`1.0.0`) |
| **License** | MIT |

Keep the exact same package name in CP — no renaming required:

```typescript
// TypeScript (npm)
import { Webhook } from "standardwebhooks";

// CP (cap)
import {
    StripeSig,
    MAX_V1_SIGS,
    parseStripeHeader,
    parseGitHubSig256,
    parseStandardWebhookSig,
    isWithinTolerance,
    constantTimeEq,
    verifyStripeExpected,
    formatStripeSignedPayload,
} from "standardwebhooks";
```

## Installation

```bash
cap add standardwebhooks
```

## Key Features & Security Highlights

- **Zero Heap Allocation**: Parses headers into stack structs (`StripeSig` with up to `MAX_V1_SIGS = 4` key-rotation `v1` signatures) and caller slices (`[]string` / `[]u8`).
- **Constant-Time Verification**: `constantTimeEq` and `verifyStripeExpected` use constant-time comparison (`cap:bin.ctEqual`) to prevent signature timing side channels.
- **Replay Attack Window Guard**: `isWithinTolerance(timestamp_secs, now_secs, tolerance_secs)` validates both past and future clock skew (`|now - ts| <= tolerance`) with safe overflow-free bounds.
- **Multi-Provider Coverage**: Supports Stripe (`t=...,v1=...`), GitHub (`sha256=<64-hex>`), and Standard Webhooks (`v1,<base64> v1,<base64>`) key-rotation signature lists.

## Quick Start

### Stripe Webhook Verification & Signed Payload Formatting

```typescript
import {
    parseStripeHeader,
    formatStripeSignedPayload,
    verifyStripeExpected,
} from "standardwebhooks";

fn handleStripeWebhook(header: string, raw_body: string, now_secs: i64, expected_hmac_hex: string): bool {
    const parsed = parseStripeHeader(header);
    if (!parsed.valid) {
        return false;
    }

    // Build "<timestamp>.<raw_body>" for HMAC-SHA256 computation
    let signed_buf: [2048]u8 = undefined;
    const signed_len = formatStripeSignedPayload(parsed.timestamp, raw_body, signed_buf[0..]);
    if (signed_len == 0) {
        return false;
    }

    // Verify timestamp tolerance (300s = 5 min) + constant-time match against any v1 signature
    return verifyStripeExpected(header, expected_hmac_hex, now_secs, 300);
}
```

### GitHub & Standard Webhooks Headers

```typescript
import {
    parseGitHubSig256,
    parseStandardWebhookSig,
    isWithinTolerance,
    constantTimeEq,
} from "standardwebhooks";

fn verifyGitHub(header: string, expected_hex: string): bool {
    const sig_hex = parseGitHubSig256(header); // returns 64-char hex or ""
    if (sig_hex.len == 0) {
        return false;
    }
    return constantTimeEq(sig_hex, expected_hex);
}

fn verifyStandardWebhook(sig_header: string, ts_secs: i64, now_secs: i64, expected_b64: string): bool {
    if (!isWithinTolerance(ts_secs, now_secs, 300)) {
        return false;
    }
    let sigs: [4]string = undefined;
    const count = parseStandardWebhookSig(sig_header, sigs[0..]);
    let i: usize = 0;
    while (i < count) {
        if (constantTimeEq(sigs[i], expected_b64)) {
            return true;
        }
        i += 1;
    }
    return false;
}
```

## API Overview

| Symbol | Signature / Value | Description |
| :--- | :--- | :--- |
| `MAX_V1_SIGS` | `4` | Max rotated `v1` signatures stored in `StripeSig` |
| `StripeSig` | `{ timestamp: i64, v1_sigs: [4]string, v1_count: usize, valid: bool }` | Parsed Stripe signature header |
| `parseStripeHeader` | `(header: string) -> StripeSig` | Parse `t=<unix>,v1=<hex>[,v1=...]` header |
| `formatStripeSignedPayload` | `(timestamp_secs: i64, payload: string, out: []u8) -> usize` | Format `"<ts>.<payload>"` into `out` (returns `0` if buffer too small) |
| `verifyStripeExpected` | `(header: string, expected_v1_hex: string, now_secs: i64, tolerance_secs: i64) -> bool` | Validate replay window + constant-time `v1` match |
| `parseGitHubSig256` | `(header: string) -> string` | Extract 64-char lowercase/uppercase hex from `sha256=<hex>` (or `""`) |
| `parseStandardWebhookSig` | `(header: string, out_sigs: []string) -> usize` | Parse space-delimited `v1,<b64>` signatures into `out_sigs` |
| `isWithinTolerance` | `(timestamp_secs: i64, now_secs: i64, tolerance_secs: i64) -> bool` | Check `\|now - ts\| <= tolerance` replay guard |
| `constantTimeEq` | `(a: string, b: string) -> bool` | Constant-time string comparison |

## License

MIT
