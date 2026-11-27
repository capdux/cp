// Type definitions for CP

import { foldAsciiByte, eqAscii } from "../../unicase/src/index.cp";
export class MediaType {
    valid: bool,
    main_type: string,
    subtype: string,
    suffix: string,
    fn validate(raw: string): bool;
    fn isValid(): bool;
    fn hasSuffix(): bool;
    fn formatTo(out: []u8): usize;
}

export fn mtIsAlphaNum(b: u8): bool;
export fn mtIsTypeChar(b: u8): bool;
export fn mtIsSuffixChar(b: u8): bool;
export fn mtIsSubtypeChar(b: u8): bool;
export fn isValidType(s: string): bool;
export fn isValidSubtype(s: string): bool;
export fn isValidSuffix(s: string): bool;
export fn mtPushLower(out: []u8, pos: usize, s: string): usize;
export fn mtStartsWithAscii(s: string, prefix: string): bool;
export fn testMediaType(raw: string): bool;
export fn testType(raw: string): bool;
export fn parseMediaType(raw: string): MediaType;
export fn parse(raw: string): MediaType;
export fn formatMediaType(main_type: string, subtype: string, suffix: string, out: []u8): usize;
export fn format(mt: MediaType, out: []u8): usize;
export fn essence(raw: string, out: []u8): usize;
export fn mediaTypeEq(a: string, b: string): bool;
export fn hasSuffix(raw: string, suffix: string): bool;
export fn isStandardType(t: string): bool;
export fn isVendorSubtype(subtype: string): bool;
export fn isPersonalSubtype(subtype: string): bool;
export fn isUnregisteredSubtype(subtype: string): bool;
