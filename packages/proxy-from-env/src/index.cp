// @cap/proxy-from-env — Zero-allocation HTTP_PROXY/HTTPS_PROXY/NO_PROXY resolver in pure CP
// Clean-room MIT implementation of npm `proxy-from-env` (#303, 169M/wk).

import { eqIgnoreCase, trim } from "cap:ascii";
import { parseU16 } from "cap:fmt";

pub struct ProxyEnv {
    pub http_proxy: string,
    pub https_proxy: string,
    pub all_proxy: string,
    pub no_proxy: string,

    pub static fn empty(): ProxyEnv {
        return ProxyEnv {
            http_proxy: "",
            https_proxy: "",
            all_proxy: "",
            no_proxy: "",
        };
    }
}

fn pfeEndsWithIgnoreCase(s: string, suffix: string): bool {
    if (suffix.len > s.len) { return false; }
    return eqIgnoreCase(s[(s.len - suffix.len)..s.len], suffix);
}

struct PfeParsedUrl {
    scheme: string,
    host: string,
    port: u16,
}

fn pfeParseUrl(url: string): PfeParsedUrl {
    let sep: usize = url.len;
    let i: usize = 0;
    while (i + 2 < url.len) {
        if (url[i] == 58 && url[i + 1] == 47 && url[i + 2] == 47) { // "://"
            sep = i;
            break;
        }
        i += 1;
    }
    if (sep >= url.len) {
        return PfeParsedUrl { scheme: "", host: "", port: 0 };
    }
    const scheme = url[0..sep];
    let auth_start = sep + 3;
    let auth_end = auth_start;
    while (auth_end < url.len && url[auth_end] != 47 && url[auth_end] != 63 && url[auth_end] != 35) {
        auth_end += 1;
    }
    // Strip userinfo@
    let at_pos = auth_start;
    while (at_pos < auth_end) {
        if (url[at_pos] == 64) {
            auth_start = at_pos + 1;
        }
        at_pos += 1;
    }
    const hostport = url[auth_start..auth_end];
    let host = hostport;
    let port: u16 = 0;

    // Handle [IPv6]:port or host:port
    if (hostport.len > 0 && hostport[0] == 91) {
        let close: usize = 1;
        while (close < hostport.len && hostport[close] != 93) { close += 1; }
        if (close < hostport.len) {
            host = hostport[1..close];
            if (close + 1 < hostport.len && hostport[close + 1] == 58) {
                port = parseU16(hostport[(close + 2)..hostport.len]) ?? 0;
            }
        }
    } else {
        let colon: usize = hostport.len;
        let k: usize = 0;
        while (k < hostport.len) {
            if (hostport[k] == 58) { colon = k; break; }
            k += 1;
        }
        if (colon < hostport.len) {
            host = hostport[0..colon];
            port = parseU16(hostport[(colon + 1)..hostport.len]) ?? 0;
        }
    }

    if (port == 0) {
        if (eqIgnoreCase(scheme, "https") || eqIgnoreCase(scheme, "wss")) {
            port = 443;
        } else if (eqIgnoreCase(scheme, "http") || eqIgnoreCase(scheme, "ws")) {
            port = 80;
        }
    }
    return PfeParsedUrl { scheme: scheme, host: host, port: port };
}

/// Evaluates whether `host` + `port` should bypass proxy according to `no_proxy` comma-separated rules.
pub fn shouldBypassProxy(host: string, port: u16, no_proxy: string): bool {
    if (no_proxy.len == 0 || host.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < no_proxy.len) {
        const start = i;
        while (i < no_proxy.len && no_proxy[i] != 44 && no_proxy[i] != 32) {
            i += 1;
        }
        const entry = trim(no_proxy[start..i]);
        while (i < no_proxy.len && (no_proxy[i] == 44 || no_proxy[i] == 32)) {
            i += 1;
        }
        if (entry.len == 0) { continue; }
        if (entry.len == 1 && entry[0] == 42) {
            return true; // '*' bypasses all
        }

        // Split optional :port on rule (preserve bare IPv6 like "::1" with >=2 colons)
        let rule_host = entry;
        let rule_port: u16 = 0;
        let colon_count: usize = 0;
        let last_colon: usize = 0;
        let c: usize = 0;
        while (c < entry.len) {
            if (entry[c] == 58) {
                colon_count += 1;
                last_colon = c;
            }
            c += 1;
        }
        if (entry.len > 0 && entry[0] == 91) { // [IPv6] or [IPv6]:port
            let rb: usize = 1;
            while (rb < entry.len && entry[rb] != 93) { rb += 1; }
            if (rb < entry.len) {
                rule_host = entry[1..rb];
                if (rb + 1 < entry.len && entry[rb + 1] == 58) {
                    rule_port = parseU16(entry[(rb + 2)..entry.len]) ?? 0;
                }
            }
        } else if (colon_count == 1) {
            rule_host = entry[0..last_colon];
            rule_port = parseU16(entry[(last_colon + 1)..entry.len]) ?? 0;
        }
        if (rule_port != 0 && rule_port != port) {
            continue;
        }

        // Strip leading "*." or "."
        let suffix_only = false;
        if (rule_host.len >= 2 && rule_host[0] == 42 && rule_host[1] == 46) {
            rule_host = rule_host[1..rule_host.len]; // keep leading '.'
            suffix_only = true;
        } else if (rule_host.len >= 1 && rule_host[0] == 46) {
            suffix_only = true;
        }

        if (suffix_only) {
            // ".example.com" matches "sub.example.com" and "example.com"
            if (pfeEndsWithIgnoreCase(host, rule_host)
                || eqIgnoreCase(host, rule_host[1..rule_host.len])) {
                return true;
            }
        } else {
            if (eqIgnoreCase(host, rule_host)) {
                return true;
            }
            // Also match subdomain boundary "<anything>.<rule_host>"
            if (host.len > rule_host.len
                && host[host.len - rule_host.len - 1] == 46
                && pfeEndsWithIgnoreCase(host, rule_host)) {
                return true;
            }
        }
    }
    return false;
}

/// Resolves the proxy URL for `target_url` given `env`, returning `""` if no proxy applies.
pub fn getProxyForUrl(target_url: string, env: ProxyEnv): string {
    const u = pfeParseUrl(target_url);
    if (u.scheme.len == 0 || u.host.len == 0) {
        return "";
    }
    if (shouldBypassProxy(u.host, u.port, env.no_proxy)) {
        return "";
    }
    if (eqIgnoreCase(u.scheme, "https") || eqIgnoreCase(u.scheme, "wss")) {
        if (env.https_proxy.len > 0) { return env.https_proxy; }
        if (env.all_proxy.len > 0) { return env.all_proxy; }
        return "";
    }
    if (eqIgnoreCase(u.scheme, "http") || eqIgnoreCase(u.scheme, "ws")) {
        if (env.http_proxy.len > 0) { return env.http_proxy; }
        if (env.all_proxy.len > 0) { return env.all_proxy; }
        return "";
    }
    if (env.all_proxy.len > 0) {
        return env.all_proxy;
    }
    return "";
}

export {
    ProxyEnv,
    shouldBypassProxy,
    getProxyForUrl,
};
