// Type definitions for CP

import { eqIgnoreCase, findByte, rfindByte } from "cap:ascii";
import { writeStr, writeU64, parseU16 } from "cap:fmt";
export class MongoHost {
    host: string,
    port: u16,
}

export class MongoUri {
    is_srv: bool,
    username: string,
    password: string,
    hosts: [8]MongoHost,
    host_count: usize,
    database: string,
    replica_set: string,
    auth_source: string,
    tls: bool,
    valid: bool,
}

export fn muStartsWithIgnoreCase(s: &[]u8, prefix: &[]u8): bool;
export fn muParsePort(s: &[]u8, out: &mut u16): bool;
export fn muWrite(out: []u8, pos: &mut usize, src: &[]u8): bool;
export fn muWriteU16(out: []u8, pos: &mut usize, val: u16): bool;
export fn muEmptyResult(): MongoUri;
export fn parseMongoUri(uri: string): MongoUri;
export fn getQueryOption(uri: string, key: string): string;
export fn formatMongoUri(cfg: MongoUri, redact_password: bool, out: []u8): usize;
