// Type definitions for CP

import { eqAscii } from "../../unicase/src/index.cp";
export fn vhIsWs(b: u8): bool;
export fn vhTrim(s: string): string;
export fn vhPush(out: []u8, pos: usize, s: string): usize;
export fn isValidFieldName(name: string): bool;
export fn hasVaryField(header: string, field: string): bool;
export fn countVaryFields(header: string): usize;
export fn appendVary(header: string, fields: string, out: []u8): usize;
