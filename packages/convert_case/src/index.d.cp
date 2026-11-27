// Type definitions for CP

import { toUpperCamelCase, toLowerCamelCase, toSnakeCase, toShoutySnakeCase, toKebabCase, toShoutyKebabCase, toTitleCase, toTrainCase } from "../../heck-case/src/index.cp";
export fn ccvReplaceUnderscoreWithSpace(out: []u8, len: usize);
export fn ccvStripUnderscores(out: []u8, len: usize): usize;
export fn toFlatCase(src: string, out: []u8): usize;
export fn toUpperFlatCase(src: string, out: []u8): usize;
export fn toLowerWords(src: string, out: []u8): usize;
export fn toUpperWords(src: string, out: []u8): usize;
export fn toAlternatingCase(src: string, out: []u8): usize;
export fn convertCase(src: string, style: u8, out: []u8): usize;
