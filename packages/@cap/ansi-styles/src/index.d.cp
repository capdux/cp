// Type definitions for CP

import { rgbToAnsi256, rgbToAnsi16, hexToRgb, keywordToRgb } from "../../color-convert/src/index.cp";
export class StylePair {
    open: string,
    close: string,
}

export fn asPushStr(out: []u8, pos: usize, s: string): usize;
export fn asPushU8(out: []u8, pos: usize, v: u8): usize;
export fn wrapStyle(style: StylePair, text: string, out: []u8): usize;
export fn formatAnsi256(code: u8, is_bg: bool, out: []u8): usize;
export fn formatAnsi16m(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize;
export fn formatRgbToAnsi16(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize;
export fn formatRgbToAnsi256(r: u8, g: u8, b: u8, is_bg: bool, out: []u8): usize;
export fn formatHexToAnsi16m(hex: string, is_bg: bool, out: []u8): usize;
export fn formatKeywordToAnsi16m(name: string, is_bg: bool, out: []u8): usize;
