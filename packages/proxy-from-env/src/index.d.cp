// Type definitions for CP

import { eqIgnoreCase, trim } from "cap:ascii";
import { parseU16 } from "cap:fmt";
export class ProxyEnv {
    http_proxy: string,
    https_proxy: string,
    all_proxy: string,
    no_proxy: string,
    fn empty(): ProxyEnv;
}

export fn pfeEndsWithIgnoreCase(s: string, suffix: string): bool;
export class PfeParsedUrl {
    scheme: string,
    host: string,
    port: u16,
}

export fn pfeParseUrl(url: string): PfeParsedUrl;
export fn shouldBypassProxy(host: string, port: u16, no_proxy: string): bool;
export fn getProxyForUrl(target_url: string, env: ProxyEnv): string;
