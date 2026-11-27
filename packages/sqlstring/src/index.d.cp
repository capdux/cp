// Type definitions for CP

import { isAlpha, isAlnum } from "cap:ascii";
import { countByte, findByteInBytes } from "cap:bin";
export fn escapeQuoted(input: string, quote: u8, allow_empty: bool, out: []u8): usize;
export fn escapeStringAnsi(input: string, out: []u8): usize;
export fn escapeIdentifierAnsi(ident: string, out: []u8): usize;
export fn escapeIdentifierMysql(ident: string, out: []u8): usize;
export fn escapeLike(pattern: string, escape_char: u8, out: []u8): usize;
export fn isSafeUnquotedIdent(ident: string): bool;
export fn countPlaceholders(sql: string): usize;
