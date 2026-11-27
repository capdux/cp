// Type definitions for CP

export class CountryMeta {
    iso2: string,
    country_code: u16,
    min_len: u8,
    max_len: u8,
    strip_leading_zero: bool,
    trunk_prefix: u8,
}

export fn toUpperAscii(c: u8): u8;
export fn isDigit(c: u8): bool;
export fn eqIso2IgnoreCase(a: string, b: string): bool;
export fn findCountryIndexByIso2(iso2: string): i32;
export fn findCountryIndexByCode(cc: u16): i32;
export fn countryCodeDigitLen(cc: u16): usize;
export fn pushByte(out: []u8, pos: usize, b: u8): usize;
export fn pushSlice(out: []u8, pos: usize, src: &[]u8): usize;
export fn pushU16(out: []u8, pos: usize, val: u16): usize;
export fn pushGroupRange(out: []u8, pos: usize, buf: [16]u8, start: usize, end: usize): usize;
export fn writeGroupedNational(out: []u8, pos: usize, cc: u16, buf: [16]u8, len: usize, sep: u8): usize;
export fn validateNationalRules(cc: u16, buf: [16]u8, len: usize): bool;
export class PhoneNumber {
    national_buf: [16]u8,
    national_len: u8,
    country_code: u16,
    iso2: string,
    is_valid: bool,
    is_possible: bool,
    fn invalid(): PhoneNumber;
    fn formatE164(out: []u8): string;
    fn formatInternational(out: []u8): string;
    fn formatNational(out: []u8): string;
    fn formatRfc3966(out: []u8): string;
}

export fn supportedCountryCount(): usize;
export fn countryCodeFromIso2(iso2: string): u16;
export fn iso2FromCountryCode(cc: u16): string;
export class CcMatch {
    idx: i32,
    cc_len: usize,
}

export fn matchCallingCodePrefix(digits: [24]u8, n: usize): CcMatch;
export fn parsePhoneNumber(raw: string, default_iso2: string): PhoneNumber;
export fn isValidPhone(raw: string, default_iso2: string): bool;
