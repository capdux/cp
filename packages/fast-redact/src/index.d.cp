// Type definitions for CP

export class RedactOptions {
    redact_emails: bool,
    redact_tokens: bool,
    redact_cards: bool,
    redact_phones: bool,
    redact_ipv4: bool,
    fn defaultAll(): RedactOptions;
    fn strict(): RedactOptions;
}

export fn isDigit(c: u8): bool;
export fn isAlpha(c: u8): bool;
export fn isAlphaNum(c: u8): bool;
export fn toLower(c: u8): u8;
export fn isTokenChar(c: u8): bool;
export fn isEmailLocalChar(c: u8): bool;
export fn isDomainChar(c: u8): bool;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn startsWithAt(s: string, idx: usize, prefix: string): bool;
export fn eqIgnoreCase(a: string, b: string): bool;
export fn maskEmail(email: string, out: []u8): string;
export fn maskPhone(phone: string, out: []u8): string;
export fn maskCard(card: string, out: []u8): string;
export fn maskSecret(secret: string, out: []u8): string;
export fn maskIp(ip: string, out: []u8): string;
export fn checkLuhnRange(s: string, start: usize, end: usize): bool;
export fn matchTokenEnd(s: string, idx: usize): usize;
export fn matchEmailEnd(s: string, idx: usize): usize;
export fn matchCardEnd(s: string, idx: usize): usize;
export fn matchPhoneEnd(s: string, idx: usize): usize;
export fn matchIpv4End(s: string, idx: usize): usize;
export fn redactText(input: string, opts: RedactOptions, out: []u8): string;
export fn containsSensitive(input: string): bool;
export fn isSensitiveKey(key: string): bool;
export fn findJsonStringEnd(s: string, open_quote: usize): usize;
export fn redactJsonFields(json_str: string, out: []u8): string;
