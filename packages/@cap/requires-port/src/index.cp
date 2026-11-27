// @cap/requires-port — Zero-heap protocol default port checker in pure CP
// Clean-room MIT implementation of npm `requires-port` (#873, 65.7M/wk).

fn rptToLower(b: u8): u8 {
    if (b >= 65 && b <= 90) { return b + 32; }
    return b;
}

fn rptEqLower(a: string, target: string): bool {
    if (a.len != target.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (rptToLower(a[i]) != target[i]) { return false; }
        i += 1;
    }
    return true;
}

fn rptNormalizeProtocol(proto: string): string {
    // Strip trailing ':' or '://'
    let end = proto.len;
    let i: usize = 0;
    while (i < proto.len) {
        if (proto[i] == 58) { // ':'
            end = i;
            break;
        }
        i += 1;
    }
    return proto[0..end];
}

/// Returns the default port for a known URL protocol, or `0` if unknown / portless (`file`).
pub fn defaultPortForProtocol(protocol: string): u16 {
    const p = rptNormalizeProtocol(protocol);
    if (rptEqLower(p, "http") || rptEqLower(p, "ws")) { return 80; }
    if (rptEqLower(p, "https") || rptEqLower(p, "wss")) { return 443; }
    if (rptEqLower(p, "ftp")) { return 21; }
    if (rptEqLower(p, "gopher")) { return 70; }
    if (rptEqLower(p, "ssh") || rptEqLower(p, "sftp")) { return 22; }
    if (rptEqLower(p, "smtp")) { return 25; }
    if (rptEqLower(p, "dns")) { return 53; }
    if (rptEqLower(p, "redis")) { return 6379; }
    if (rptEqLower(p, "postgres") || rptEqLower(p, "postgresql")) { return 5432; }
    if (rptEqLower(p, "mysql")) { return 3306; }
    return 0;
}

/// Returns `true` if `port` must be explicitly included in a URL for `protocol`
/// (i.e. `port != 0` and `port` differs from `protocol`'s default port).
pub fn requiresPort(port: u16, protocol: string): bool {
    if (port == 0) {
        return false;
    }
    const p = rptNormalizeProtocol(protocol);
    if (rptEqLower(p, "file")) {
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
