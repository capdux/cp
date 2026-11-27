// Type definitions for CP

import { toCamelCase, toSnakeCase, toPascalCase } from "../../camelcase/src/index.cp";
export fn ckIsSpace(ch: u8): bool;
export fn ckWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn ckWriteSlice(out: []u8, pos: usize, src: &[]u8): usize;
export fn ckConvertJsonKeys(json: string, mode: u8, out: []u8): usize;
export fn convertJsonKeysToCamel(json: string, out: []u8): usize;
export fn convertJsonKeysToSnake(json: string, out: []u8): usize;
export fn convertJsonKeysToPascal(json: string, out: []u8): usize;
