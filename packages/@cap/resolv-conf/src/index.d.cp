// Type definitions for CP

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

export fn rcfIsSpace(b: u8): bool;
export fn rcfStrEq(a: string, b: string): bool;
export fn rcfStartsWith(s: string, prefix: string): bool;
export fn rcfParseU32(s: string): u32?;
export fn defaultResolvConfig(): ResolvConfig;
export fn parseResolvConf(content: string): ResolvConfig;
