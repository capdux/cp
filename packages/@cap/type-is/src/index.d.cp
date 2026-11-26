// Type definitions for CP

import { MediaType, parse, testMediaType } from "../../media-typer/src/index.cp";
export fn tiIsSpace(c: u8): bool;
export fn tiToLowerAscii(c: u8): u8;
export fn tiTrimSlice(s: &[]u8): &[]u8;
export fn tiEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn tiIsStar(s: &[]u8): bool;
export fn tiFindSingleSlash(s: &[]u8): i64;
export fn tiMatchStructuredSuffix(ct: &[]u8, parsed: MediaType, plusSuffix: &[]u8): bool;
export fn stripParams(content_type: &[]u8): &[]u8;
export fn normalizeType(short_or_mime: &[]u8): &[]u8;
export fn mimeMatch(content_type: &[]u8, pattern: &[]u8): bool;
export fn matchAny(content_type: &[]u8, patterns: &[]&[]u8): i64;
