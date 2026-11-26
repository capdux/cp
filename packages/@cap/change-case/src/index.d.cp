// Type definitions for CP

import { toCamelCase, toPascalCase } from "../../camelcase/src/index.cp";
import { decamelize } from "../../decamelize/src/core.cp";
export fn ccaseIsUpper(c: u8): bool;
export fn ccaseIsLower(c: u8): bool;
export fn ccaseIsDigit(c: u8): bool;
export fn ccaseIsAlnum(c: u8): bool;
export fn ccaseToUpper(c: u8): u8;
export fn ccaseToLower(c: u8): u8;
export fn ccaseJoinLower(input: string, sep: string, out: []u8): usize;
export fn ccaseCapitalizeWords(out: []u8, len: usize, sep_byte: u8);
export fn camelCase(input: string, out: []u8): usize;
export fn pascalCase(input: string, out: []u8): usize;
export fn snakeCase(input: string, out: []u8): usize;
export fn kebabCase(input: string, out: []u8): usize;
export fn constantCase(input: string, out: []u8): usize;
export fn dotCase(input: string, out: []u8): usize;
export fn pathCase(input: string, out: []u8): usize;
export fn trainCase(input: string, out: []u8): usize;
export fn capitalCase(input: string, out: []u8): usize;
export fn sentenceCase(input: string, out: []u8): usize;
export fn noCase(input: string, out: []u8): usize;
export fn swapCase(input: string, out: []u8): usize;
