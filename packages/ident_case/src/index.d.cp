// Type definitions for CP

import { isDigit, isAlnum, toLowerByte, toUpperByte, strEq } from "cap:ascii";

export enum RenameRule {
    LowerCase,
    UpperCase,
    CamelCase,
    PascalCase,
    SnakeCase,
    ScreamingSnakeCase,
    KebabCase,
    ScreamingKebabCase
}

export fn toLowerCase(input: string, out: []u8): usize;
export fn toUpperCase(input: string, out: []u8): usize;
export fn toCamelCase(input: string, out: []u8): usize;
export fn toPascalCase(input: string, out: []u8): usize;
export fn toSnakeCase(input: string, out: []u8): usize;
export fn toKebabCase(input: string, out: []u8): usize;
export fn toScreamingSnakeCase(input: string, out: []u8): usize;
export fn toScreamingKebabCase(input: string, out: []u8): usize;
export fn renameIdent(rule: RenameRule, input: string, out: []u8): usize;
export fn parseRenameRule(name: string): RenameRule?;
