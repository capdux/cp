# standardwebhooks@1.0.0 (Agent Quickref)

> Zero-allocation Stripe, GitHub (X-Hub-Signature-256), and Standard Webhooks signature parser, replay window guard, and constant-time verifier in pure CP | License: MIT
> Import: `import { ... } from "standardwebhooks";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseNonNegI64(s: string): i64;`
- `export fn parseStripeHeader(header: string): StripeSig;`
- `export fn parseGitHubSig256(header: string): string;`
- `export fn parseStandardWebhookSig(header: string, out_sigs: []string): usize;`
- `export fn isWithinTolerance(timestamp_secs: i64, now_secs: i64, tolerance_secs: i64): bool;`
- `export fn constantTimeEq(a: string, b: string): bool;`
- `export fn verifyStripeExpected(header: string, expected_v1_hex: string, now_secs: i64, tolerance_secs: i64): bool;`
- `export fn formatStripeSignedPayload(timestamp_secs: i64, payload: string, out: []u8): usize;`
