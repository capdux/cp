// Type definitions for CP

export class StripeSig {
    timestamp: i64,
    v1_sigs: [4]string,
    v1_count: usize,
    valid: bool,
}

export fn isSpace(b: u8): bool;
export fn isHex(b: u8): bool;
export fn trimSlice(s: string): string;
export fn parseNonNegI64(s: string): i64;
export fn parseStripeHeader(header: string): StripeSig;
export fn parseGitHubSig256(header: string): string;
export fn parseStandardWebhookSig(header: string, out_sigs: []string): usize;
export fn isWithinTolerance(timestamp_secs: i64, now_secs: i64, tolerance_secs: i64): bool;
export fn constantTimeEq(a: string, b: string): bool;
export fn verifyStripeExpected(header: string, expected_v1_hex: string, now_secs: i64, tolerance_secs: i64): bool;
export fn formatStripeSignedPayload(timestamp_secs: i64, payload: string, out: []u8): usize;
