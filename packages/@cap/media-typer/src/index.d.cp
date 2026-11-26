// Type definitions for CP

import { foldAsciiByte, eqAscii } from "../../unicase/src/index.cp";
export class MediaType {
    valid: bool,
    main_type: string,
    subtype: string,
    suffix: string,
}

export fn mtIsAlphaNum(b: u8): bool;
export fn mtIsRestrictedChar(b: u8): bool;
export fn mtIsValidName(s: string): bool;
export fn mtPushLower(out: []u8, pos: usize, s: string): usize;
export fn testMediaType(raw: string): bool;
export fn parseMediaType(raw: string): MediaType;
export fn formatMediaType(main_type: string, subtype: string, suffix: string, out: []u8): usize;
export fn mediaTypeEq(a: string, b: string): bool;
