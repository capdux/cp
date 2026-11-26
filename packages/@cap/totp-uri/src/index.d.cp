// Type definitions for CP

export class OtpAuthUri {
    kind: u8,
    label: string,
    secret: string,
    issuer: string,
    algorithm: u8,
    digits: u8,
    period: u32,
    counter: u64,
    valid: bool,
}

export fn otpToLower(c: u8): u8;
export fn otpEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn otpIsUnreserved(c: u8): bool;
export fn otpHexUpper(n: u8): u8;
export fn otpParseU64(s: &[]u8, out: &mut u64): bool;
export fn otpWriteRaw(out: []u8, pos: &mut usize, src: &[]u8): bool;
export fn otpWritePctEncoded(out: []u8, pos: &mut usize, src: &[]u8, allow_label_chars: bool): bool;
export fn otpWriteU64(out: []u8, pos: &mut usize, val: u64): bool;
export fn isValidBase32Secret(secret: string): bool;
export fn parseOtpAuthUri(uri: string): OtpAuthUri;
export fn formatOtpAuthUri(cfg: OtpAuthUri, out: []u8): usize;
export class LabelParts {
    issuer: string,
    account: string,
}

export fn splitLabel(label: string): LabelParts;
