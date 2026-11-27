// Type definitions for CP

import { eqIgnoreCase, findByte, rfindByte } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";
export class AmqpConfig {
    tls: bool,
    host: string,
    port: u16,
    username: string,
    password: string,
    vhost: string,
    heartbeat: u16,
    valid: bool,
}

export fn invalidAmqp(): AmqpConfig;
export fn amqpIsSpace(b: u8): bool;
export fn parseU16Strict(s: string): i32;
export fn parseAmqpUrl(url: string): AmqpConfig;
export fn pushSlice(dst: []u8, pos: usize, src: string): i64;
export fn pushU16(dst: []u8, pos: usize, val: u16): i64;
export fn formatAmqpUrl(cfg: AmqpConfig, redact_password: bool, out: []u8): usize;
