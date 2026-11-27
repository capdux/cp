// Type definitions for CP

import { normalizeNfc } from "../../icu_normalizer/src/index.cp";
import { encodeLabel, decodeLabel, isAsciiOnly } from "../../punycode/src/index.cp";
import { toLowerByte, isAlnum } from "cap:ascii";
export fn idnaIsLdhLower(ch: u8): bool;
export fn idnaIsLdhAnyCase(ch: u8): bool;
export fn isPunycodeLabel(label: string): bool;
export fn idnaMapDomain(domain: string, out: []u8): i64;
export fn idnaValidateAsciiLabelLower(label: &[]u8): i64;
export fn idnaAppendBytes(out: []u8, pos: &mut usize, src: &[]u8): bool;
export fn domainToAscii(domain: string, out: []u8): i64;
export fn domainToUnicode(domain: string, out: []u8): i64;
export fn isValidIdnaAscii(domain: string): bool;
