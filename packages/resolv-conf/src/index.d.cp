// Type definitions for CP

import { isSpace, strEq, startsWith } from "cap:ascii";
import { parseU32 } from "cap:fmt";
export class ResolvConfig {
    nameservers: [8]string,
    nameserver_count: usize,
    search_domains: [8]string,
    search_count: usize,
    domain: string,
    ndots: u32,
    timeout_sec: u32,
    attempts: u32,
    rotate: bool,
    edns0: bool,
    use_vc: bool,
    trust_ad: bool,
}

export fn defaultResolvConfig(): ResolvConfig;
export fn parseResolvConf(content: string): ResolvConfig;
