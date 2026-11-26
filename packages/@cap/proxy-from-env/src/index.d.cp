// Type definitions for CP

export class ProxyEnv {
    http_proxy: string,
    https_proxy: string,
    all_proxy: string,
    no_proxy: string,
    fn empty(): ProxyEnv;
}

export fn pfeToLower(b: u8): u8;
export fn pfeEqIgnoreCase(a: string, b: string): bool;
export fn pfeEndsWithIgnoreCase(s: string, suffix: string): bool;
export fn pfeTrim(s: string): string;
export class PfeParsedUrl {
    scheme: string,
    host: string,
    port: u16,
}

export fn pfeParseU16(s: string): u16;
export fn pfeParseUrl(url: string): PfeParsedUrl;
export fn shouldBypassProxy(host: string, port: u16, no_proxy: string): bool;
export fn getProxyForUrl(target_url: string, env: ProxyEnv): string;
