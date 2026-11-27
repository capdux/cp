// Type definitions for CP

import { strEq, startsWith, findByte, rfindByte, trim } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
export class PgConfig {
    host: string,
    port: u16,
    user: string,
    password: string,
    database: string,
    sslmode: string,
    valid: bool,
}

export fn makeInvalid(): PgConfig;
export fn isWs(c: u8): bool;
export fn parsePortStrict(s: string): i32;
export fn isValidSslMode(mode: string): bool;
export fn parseUri(uri: string): PgConfig;
export fn parseKeyValue(dsn: string): PgConfig;
export fn parse(input: string): PgConfig;
export fn appendSlice(out: []u8, pos: usize, s: string): i64;
export fn appendU16(out: []u8, pos: usize, v: u16): i64;
export fn formatUri(cfg: PgConfig, include_password: bool, out: []u8): usize;
