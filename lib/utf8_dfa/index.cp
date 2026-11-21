// Source: Absorbed from /data/dev/bun/src/unicode/uucode_lib/src/utf8.cp, /data/dev/bun/src/unicode/uucode_lib/src/ascii.cp
import dfa from "./dfa.cp";

export const UTF8_ACCEPT = dfa.UTF8_ACCEPT;
export const UTF8_REJECT = dfa.UTF8_REJECT;
export const REPLACEMENT_CHAR = dfa.REPLACEMENT_CHAR;
export const DecodeStep = dfa.DecodeStep;
export const decodeByte = dfa.decodeByte;
export const isDoneDecoding = dfa.isDoneDecoding;
export const Utf8Iterator = dfa.Utf8Iterator;
export const isValidUtf8 = dfa.isValidUtf8;
export const countCodepoints = dfa.countCodepoints;
export const isAscii = dfa.isAscii;
export const isDigit = dfa.isDigit;
export const isUpper = dfa.isUpper;
export const isLower = dfa.isLower;
export const isAlphabetic = dfa.isAlphabetic;
export const isAlphanumeric = dfa.isAlphanumeric;
export const isHex = dfa.isHex;
export const isControl = dfa.isControl;
export const isPrint = dfa.isPrint;
export const isWhitespace = dfa.isWhitespace;
export const toUpper = dfa.toUpper;
export const toLower = dfa.toLower;
