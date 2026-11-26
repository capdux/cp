// Type definitions for CP

export fn ajvfIsDigit(b: u8): bool;
export fn ajvfIsAlpha(b: u8): bool;
export fn ajvfIsAlphaNum(b: u8): bool;
export fn ajvfIsHex(b: u8): bool;
export fn ajvfParse2Digits(s: string, offset: usize): i32;
export fn ajvfParse4Digits(s: string, offset: usize): i32;
export fn ajvfIsLeapYear(year: u32): bool;
export fn ajvfDaysInMonth(year: u32, month: u32): u32;
export fn isDate(s: string): bool;
export fn isTime(s: string): bool;
export fn isDateTime(s: string): bool;
export fn isHostname(s: string): bool;
export fn ajvfIsEmailLocalChar(b: u8): bool;
export fn isEmail(s: string): bool;
export fn isIpv4(s: string): bool;
export fn isIpv6(s: string): bool;
export fn isUuid(s: string): bool;
export fn ajvfIsSchemeChar(b: u8): bool;
export fn isUri(s: string): bool;
export fn isJsonPointer(s: string): bool;
export fn ajvfStrEq(a: string, b: string): bool;
export fn validateFormat(format_name: string, value: string): bool;
