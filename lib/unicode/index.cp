// Source: bun/src/unicode/uucode_lib/src/root.cp -> cp/lib/unicode/index.cp (cap:unicode public entry)

export * from "./utf.cp";
export * from "./props.cp";
export * from "./grapheme.cp";
export * from "./width.cp";
export * from "./lut.cp";

import r from "./range.cp";
export const MAX_CODEPOINT = r.MAX_CODEPOINT;
export const UnicodeRangeError = r.UnicodeRangeError;
export const UnicodeRange = r.UnicodeRange;

export {
    isZeroWidth,
    isWideChar,
    strDisplayWidth,
    sliceToDisplayWidth,
} from "./wcwidth.cp";
import wc from "./wcwidth.cp";
export const wcwidth = wc.codepointWidth;
