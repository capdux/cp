// Type definitions for CP

import { strEq, hexVal } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { encodeCodepointLossy } from "cap:utf8";
export fn mapInvalidCharRef(cp: u32): u32;
export fn decodeNamedEntity(name: string): u32?;
export fn decodeNumericEntity(body: string): u32?;
export fn decodeEntities(input: string, out: []u8): usize;
export fn encodeEntities(input: string, out: []u8): usize;
