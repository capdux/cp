// Type definitions for CP

import { bytesEq, findByteInBytes } from "cap:bin";
import { writeStr, writeI64, parseI64 } from "cap:fmt";
import { toCamelCase } from "../../ident_case/src/index.cp";
import { decamelize } from "../../decamelize/src/core.cp";
export class ParsedItem {
    data: [128]u8,
    len: usize,
}

export class ParsedEntry {
    key: [64]u8,
    key_len: usize,
    camel_key: [64]u8,
    camel_len: usize,
    kebab_key: [64]u8,
    kebab_len: usize,
    kind: u8,
    bool_val: bool,
    int_val: i64,
    str_val: [128]u8,
    str_len: usize,
}

export fn ypIsNegativeNumber(s: string): bool;
export fn ypFormatInt(valIn: i64, out: []u8): i64;
export fn ypIsFlagToken(s: string): bool;
export fn ypEmptyItem(): ParsedItem;
export fn ypEmptyEntry(): ParsedEntry;
export fn ypEntryMatchesKey(entry: ParsedEntry, query: string): bool;
export class ParsedArgs {
    entries: [32]ParsedEntry,
    entry_count: usize,
    positionals: [32]ParsedItem,
    pos_count: usize,
    rest: [16]ParsedItem,
    rest_count: usize,
    fn findIndex(key: string): i64;
    fn has(key: string): bool;
    fn getBool(key: string, default_val: bool): bool;
    fn getInt(key: string, default_val: i64): i64;
    fn getString(key: string, out: []u8): i64;
    fn entryCount(): usize;
    fn positionalCount(): usize;
    fn positionalAt(idx: usize, out: []u8): usize;
    fn restCount(): usize;
    fn restAt(idx: usize, out: []u8): usize;
}

export fn ypPushPositional(args: &mut ParsedArgs, s: string);
export fn ypPushRest(args: &mut ParsedArgs, s: string);
export fn ypSetEntry(args: &mut ParsedArgs, rawKey: string, kind: u8, boolVal: bool, intVal: i64, strVal: string);
export fn ypSetCoerced(args: &mut ParsedArgs, rawKey: string, rawVal: string);
export fn parseArgs(argv: &[]string): ParsedArgs;
