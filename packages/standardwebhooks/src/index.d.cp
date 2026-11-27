// Type definitions for CP

import { isSpace, isDigit, isHexDigit, trim, startsWith, strEq, findByte } from "cap:ascii";
import { ctEqual } from "cap:bin";
import { parseI64, writeStr, writeU64 } from "cap:fmt";
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
