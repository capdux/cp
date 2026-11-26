// Type definitions for CP

export class MediaType {
    type_start: usize,
    type_len: usize,
    subtype_start: usize,
    subtype_len: usize,
    suffix_start: usize,
    suffix_len: usize,
    has_suffix: bool,
    valid: bool,
}

export fn mtInvalid(): MediaType;
export fn mtIsAlphaNum(c: u8): bool;
export fn mtIsTypeChar(c: u8): bool;
export fn mtIsSubtypeChar(c: u8): bool;
export fn mtIsValidTypeSlice(s: &[]u8): bool;
export fn isValidToken(s: string): bool;
export fn parse(input: string): MediaType;
export fn testMediaType(input: string): bool;
export fn format(t: string, subtype: string, suffix: string, out: []u8): usize;
