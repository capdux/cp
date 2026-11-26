// packages/@cap/spiffe-id/src/index.cp
// Zero-allocation CNCF SPIFFE ID parser, validator & formatter.
// MIT clean-room pure CP implementation.

/// Zero-copy parsed CNCF SPIFFE ID (`spiffe://<trust-domain>[/<workload-path>]`).
pub struct SpiffeId {
    pub trust_domain: string,
    pub path: string,
    pub valid: bool,
}

fn sidEqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn sidIsTrustDomainChar(c: u8): bool {
    if ((c >= ('a' as u8) && c <= ('z' as u8)) || (c >= ('0' as u8) && c <= ('9' as u8))) {
        return true;
    }
    return c == ('.' as u8) || c == ('-' as u8) || c == ('_' as u8);
}

fn sidIsPathSegmentChar(c: u8): bool {
    if (sidIsTrustDomainChar(c)) {
        return true;
    }
    return c >= ('A' as u8) && c <= ('Z' as u8);
}

/// Validate a SPIFFE trust domain per CNCF SPIFFE spec:
/// length 1..255, strictly lowercase `a-z`, `0-9`, `.`, `-`, `_`.
pub fn isValidTrustDomain(td: string): bool {
    if (td.len == 0 || td.len > 255) {
        return false;
    }
    let i: usize = 0;
    while (i < td.len) {
        if (!sidIsTrustDomainChar(td[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Validate a SPIFFE workload path per CNCF SPIFFE spec:
/// - Empty string `""` is valid.
/// - Non-empty path MUST start with `'/'`, MUST NOT end with `'/'`.
/// - MUST NOT contain empty segments (`//`), `.` segments, or `..` segments.
/// - Segment characters restricted to `a-z`, `A-Z`, `0-9`, `.`, `-`, `_`.
pub fn isValidPath(path: string): bool {
    if (path.len == 0) {
        return true;
    }
    if (path[0] != ('/' as u8)) {
        return false;
    }
    if (path[path.len - 1] == ('/' as u8)) {
        return false;
    }

    let pos: usize = 1;
    while (pos < path.len) {
        let segEnd = pos;
        while (segEnd < path.len && path[segEnd] != ('/' as u8)) {
            if (!sidIsPathSegmentChar(path[segEnd])) {
                return false;
            }
            segEnd += 1;
        }

        const segLen = segEnd - pos;
        if (segLen == 0) {
            return false;
        }
        if (segLen == 1 && path[pos] == ('.' as u8)) {
            return false;
        }
        if (segLen == 2 && path[pos] == ('.' as u8) && path[pos + 1] == ('.' as u8)) {
            return false;
        }

        if (segEnd < path.len) {
            pos = segEnd + 1;
        } else {
            pos = path.len;
        }
    }
    return true;
}

/// Strictly parse and validate a CNCF SPIFFE ID URI (`spiffe://<trust-domain>[/<path>]`).
/// Enforces lowercase `spiffe://` scheme, max length 2048 bytes, valid trust domain, and valid path.
pub fn parseSpiffeId(uri: string): SpiffeId {
    let invalid = SpiffeId{
        trust_domain: "",
        path: "",
        valid: false,
    };

    if (uri.len <= 9 || uri.len > 2048) {
        return invalid;
    }
    if (!sidEqBytes(uri[0..9], "spiffe://")) {
        return invalid;
    }

    const rest = uri[9..uri.len];
    let slash = rest.len;
    let i: usize = 0;
    while (i < rest.len) {
        if (rest[i] == ('/' as u8)) {
            slash = i;
            break;
        }
        i += 1;
    }

    const td = rest[0..slash];
    let pathSlice: []const u8 = "";
    if (slash < rest.len) {
        pathSlice = rest[slash..rest.len];
    }

    if (!isValidTrustDomain(td) || !isValidPath(pathSlice)) {
        return invalid;
    }

    return SpiffeId{
        trust_domain: td,
        path: pathSlice,
        valid: true,
    };
}

/// Returns true if `id` is valid and belongs to `trust_domain`.
pub fn belongsToTrustDomain(id: SpiffeId, trust_domain: string): bool {
    if (!id.valid || !isValidTrustDomain(trust_domain)) {
        return false;
    }
    return sidEqBytes(id.trust_domain, trust_domain);
}

/// Format a validated SPIFFE ID into `out` as `spiffe://<trust_domain><path>`.
/// Returns bytes written, or `0` on invalid components, >2048 bytes, or insufficient `out` buffer.
pub fn formatSpiffeId(trust_domain: string, path: string, out: []u8): usize {
    if (!isValidTrustDomain(trust_domain) || !isValidPath(path)) {
        return 0;
    }
    const total = 9 + trust_domain.len + path.len;
    if (total > 2048 || total > out.len) {
        return 0;
    }

    const prefix = "spiffe://";
    let pos: usize = 0;
    let pi: usize = 0;
    while (pi < 9) {
        out[pos] = prefix[pi];
        pos += 1;
        pi += 1;
    }
    let ti: usize = 0;
    while (ti < trust_domain.len) {
        out[pos] = trust_domain[ti];
        pos += 1;
        ti += 1;
    }
    let xi: usize = 0;
    while (xi < path.len) {
        out[pos] = path[xi];
        pos += 1;
        xi += 1;
    }
    return pos;
}

export {
    SpiffeId,
    isValidTrustDomain,
    isValidPath,
    parseSpiffeId,
    belongsToTrustDomain,
    formatSpiffeId,
};
