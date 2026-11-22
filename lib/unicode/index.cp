// Source: bun/src/unicode/uucode_lib/src/root.cp -> cp/lib/unicode/index.cp (cap:unicode public entry)

export * from "./utf.cp";
export * from "./props.cp";
export * from "./grapheme.cp";
export * from "./width.cp";
export * from "./lut.cp";
export * from "./ascii.cp";

import r from "./range.cp";
pub const MAX_CODEPOINT = r.MAX_CODEPOINT;
pub const UnicodeRangeError = r.UnicodeRangeError;
pub const UnicodeRange = r.UnicodeRange;

export {
    isZeroWidth,
    isWideChar,
    strDisplayWidth,
    sliceToDisplayWidth,
} from "./wcwidth.cp";
import wc from "./wcwidth.cp";
pub const wcwidth = wc.codepointWidth;
