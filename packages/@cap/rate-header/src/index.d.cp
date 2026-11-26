// Type definitions for CP

export class RateLimitPolicy {
    quota: u64,
    window_secs: u64,
    burst: u64,
    has_burst: bool,
    valid: bool,
}

export class RateLimitState {
    limit: u64,
    remaining: u64,
    reset_secs: u64,
    valid: bool,
}

export fn rhToLower(c: u8): u8;
export fn rhIsOws(c: u8): bool;
export fn rhTrimOws(s: &[]u8): &[]u8;
export fn rhEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn rhFormatU64(val: u64, buf: []u8): usize;
export fn rhCopyInto(dst: []u8, pos: &mut usize, src: &[]u8);
export fn parseUintHeader(value: string): i64;
export fn parsePolicy(header: string): RateLimitPolicy;
export fn formatPolicy(quota: u64, window_secs: u64, out: []u8): usize;
export fn parseStructuredRateLimit(header: string): RateLimitState;
export fn formatStructuredRateLimit(state: RateLimitState, out: []u8): usize;
export fn isExhausted(state: RateLimitState): bool;
export fn computeResetDelaySecs(reset_val: u64, now_unix_secs: u64): u64;
