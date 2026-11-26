// Type definitions for CP

export class JwtParts {
    header_b64: string,
    payload_b64: string,
    signature_b64: string,
    signing_input: string,
    valid: bool,
}

export fn invalidParts(): JwtParts;
export fn b64UrlVal(b: u8): i32;
export fn splitToken(token: string): JwtParts;
export fn decodeBase64Url(input: string, out: []u8): i64;
export fn decodeHeader(token: string, out: []u8): i64;
export fn decodePayload(token: string, out: []u8): i64;
export fn isExpired(exp_unix: i64, now_unix: i64, leeway_secs: i64): bool;
export fn isNotYetValid(nbf_unix: i64, now_unix: i64, leeway_secs: i64): bool;
export fn isJsonSpace(b: u8): bool;
export fn toLowerAscii(b: u8): u8;
export fn isAlgNone(header_json: string): bool;
