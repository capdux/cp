// @cap/requires-port — Zero-heap protocol default port checker in pure CP
// Clean-room MIT implementation of npm `requires-port` (#873, 65.7M/wk).

import { eqIgnoreCase, findByte } from "cap:ascii";

fn rptNormalizeProtocol(proto: string): string {
    // Strip trailing ':' or '://'
    if (idx := findByte(proto, ':', 0)) {
        return proto[0..idx];
    }
    return proto;
}

/// Returns the default port for a known URL protocol, or `0` if unknown / portless (`file`).
pub fn defaultPortForProtocol(protocol: string): u16 {
    const p = rptNormalizeProtocol(protocol);
    if (eqIgnoreCase(p, "http") || eqIgnoreCase(p, "ws")) { return 80; }
    if (eqIgnoreCase(p, "https") || eqIgnoreCase(p, "wss")) { return 443; }
    if (eqIgnoreCase(p, "ftp")) { return 21; }
    if (eqIgnoreCase(p, "gopher")) { return 70; }
    if (eqIgnoreCase(p, "ssh") || eqIgnoreCase(p, "sftp")) { return 22; }
    if (eqIgnoreCase(p, "smtp")) { return 25; }
    if (eqIgnoreCase(p, "dns")) { return 53; }
    if (eqIgnoreCase(p, "redis")) { return 6379; }
    if (eqIgnoreCase(p, "postgres") || eqIgnoreCase(p, "postgresql")) { return 5432; }
    if (eqIgnoreCase(p, "mysql")) { return 3306; }
    return 0;
}

/// Returns `true` if `port` must be explicitly included in a URL for `protocol`
/// (i.e. `port != 0` and `port` differs from `protocol`'s default port).
pub fn requiresPort(port: u16, protocol: string): bool {
    if (port == 0) {
        return false;
    }
    const p = rptNormalizeProtocol(protocol);
    if (eqIgnoreCase(p, "file")) {
        return false;
    }
    const def = defaultPortForProtocol(p);
    if (def == 0) {
        return true;
    }
    return port != def;
}

export {
    defaultPortForProtocol,
    requiresPort,
};
