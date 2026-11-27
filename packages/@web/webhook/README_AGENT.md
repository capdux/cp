# @web/webhook@1.0.0 (Agent Quickref)

> Official Stripe / GitHub / Standard Webhooks HMAC-SHA256 verification and replay protection plugin for cap:web | License: MIT
> Import: `import { ... } from "@web/webhook";` | Full types: `src/index.d.cp`

## Functions
- `export fn whRotr32(x: u32, n: u5): u32;`
- `export fn whSha256Compress(state: &mut [8]u32, block: &[64]u8);`
- `export fn whSha256(data: &[]u8, out: &mut [32]u8);`
- `export fn whHmacSha256(key: string, msg: &[]u8, out: &mut [32]u8);`
- `export fn whAsciiLower(c: u8): u8;`
- `export fn whStrEql(a: string, b: string): bool;`
- `export fn whAsciiEqlIgnoreCase(a: string, b: string): bool;`
- `export fn whConstTimeHexEql(expected_lower: &[]u8, candidate: string): bool;`
- `export fn whParseU64(s: string): u64?;`
- `export fn whNowSec(): u64;`
- `export fn whHasSeenEvent(evt_id: string): bool;`
- `export fn whRecordEvent(evt_id: string);`
- `export fn webhookSignHex(secret: string, payload: string, out_hex: []u8): usize?;`
- `export fn webhookVerifySignature(secret: string, payload: string, header_sig: string): bool;`
- `export fn setWebhookTestClock(now_sec: u64);`
- `export fn resetWebhookStore();`
- `export fn whSeenCount(): usize;`
- `export fn whBeforeHandle(ctx: &mut Context): bool;`
- `export fn whDefaultHandler(ctx: &mut Context): Response;`
- `export fn whCreatePlugin(cfg: WebhookConfig): Plugin;`
- `export fn registerWebhook(app: &mut App, cfg: WebhookConfig): &mut App;`
