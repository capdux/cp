// Type definitions for CP

import { hexCharLower } from "cap:ascii";
import { bytesEq } from "cap:bin";
import { writeStr, writeI64 } from "cap:fmt";
export fn isServerError(code: i32): bool;
export fn errorMessage(code: i32): string;
export fn isWs(c: u8): bool;
export fn appendByte(out: []u8, pos: usize, b: u8): i64;
export fn appendSlice(out: []u8, pos: usize, s: string): i64;
export fn appendI32(out: []u8, pos: usize, v: i32): i64;
export fn appendJsonString(out: []u8, pos: usize, s: string): i64;
export fn encodeRequest(id_json: string, method: string, params_json: string, out: []u8): usize;
export fn encodeResult(id_json: string, result_json: string, out: []u8): usize;
export fn encodeError(id_json: string, code: i32, message: string, data_json: string, out: []u8): usize;
export fn isBatch(json: string): bool;
export fn isValidEnvelope(json: string): bool;
