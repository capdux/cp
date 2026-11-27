// Type definitions for CP

import { startsWith, findByte } from "cap:ascii";
import { writeStr, writeU64, parseU16 } from "cap:fmt";
export class ProxyEndpoint {
    scheme: string,
    host: string,
    port: u16,
    username: string,
    password: string,
    has_auth: bool,
    is_tls: bool,
    valid: bool,
}

export fn hpaParsePort(s: string): u16?;
export fn parseProxyUrl(url: string): ProxyEndpoint;
export fn hpaWriteBase64(input: &[]u8, out: []u8, pos: usize): usize;
export fn buildConnectRequest(target_host: string, target_port: u16, proxy: ProxyEndpoint, out: []u8): usize;
