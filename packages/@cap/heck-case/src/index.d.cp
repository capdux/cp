// Type definitions for CP

export class HckRune {
    cp: u32,
    len: usize,
}

export fn hckDecodeUtf8(src: string, offset: usize): HckRune;
export fn hckClassify(cp: u32): u8;
export fn hckToLowerCp(cp: u32): u32;
export fn hckToUpperCp(cp: u32): u32;
export fn hckEncodeUtf8(cp: u32, out: []u8, pos: usize): usize;
export fn hckEmitWord(src: string, start: usize, end: usize, out: []u8, wpos: usize, word_idx: usize, sep: u8, style: u8): usize;
export fn hckConvert(src: string, out: []u8, sep: u8, style: u8): usize;
export fn toUpperCamelCase(src: string, out: []u8): usize;
export fn toLowerCamelCase(src: string, out: []u8): usize;
export fn toSnakeCase(src: string, out: []u8): usize;
export fn toShoutySnakeCase(src: string, out: []u8): usize;
export fn toKebabCase(src: string, out: []u8): usize;
export fn toShoutyKebabCase(src: string, out: []u8): usize;
export fn toTitleCase(src: string, out: []u8): usize;
export fn toTrainCase(src: string, out: []u8): usize;
