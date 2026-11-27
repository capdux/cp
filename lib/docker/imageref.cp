// Source: Absorbed from /data/dev/bun/src/cli.zig | Synced: 2026-11-21T15:24:58Z (OCI / Docker Image Reference Parser)
// Pure-CP zero-allocation OCI / Docker image reference parser (`[registry/]repository[:tag][@digest]`).

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsByte(s: []const u8, b: u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == b) return true;
        i += 1;
    }
    return false;
}

fn isValidTag(tag: []const u8): bool {
    if (tag.len == 0 || tag.len > 128) return false;
    if (tag[0] == '.' || tag[0] == '-') return false;
    let i: usize = 0;
    while (i < tag.len) {
        const c = tag[i];
        const ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '_' || c == '.' || c == '-';
        if (!ok) return false;
        i += 1;
    }
    return true;
}

fn isHexLower(b: u8): bool {
    return (b >= '0' && b <= '9') || (b >= 'a' && b <= 'f');
}

/// Validate an OCI digest (`sha256:<64 hex>` or `sha512:<128 hex>`).
export fn isValidOciDigest(digest: []const u8): bool {
    let colon: usize? = null;
    let i: usize = 0;
    while (i < digest.len) {
        if (digest[i] == ':') {
            colon = i;
            break;
        }
        i += 1;
    }
    const cp = colon ?? return false;
    const alg = digest[0..cp];
    const hex = digest[cp + 1..];
    if (strEq(alg, "sha256")) {
        if (hex.len != 64) return false;
    } else if (strEq(alg, "sha512")) {
        if (hex.len != 128) return false;
    } else {
        return false;
    }
    let k: usize = 0;
    while (k < hex.len) {
        if (!isHexLower(hex[k])) return false;
        k += 1;
    }
    return true;
}

export struct DockerImageRef {
    registry: ([]const u8)?,   // null means default Docker Hub ("docker.io")
    repository: []const u8,    // e.g. "nginx" or "cp-lang/cpc"
    tag: ([]const u8)?,        // e.g. "alpine" or null if omitted
    digest: ([]const u8)?,     // e.g. "sha256:..."

    /// Parse an OCI/Docker image reference: `[registry[:port]/]repository[:tag][@digest]`.
    pub static fn parse(raw: []const u8): DockerImageRef? {
        if (raw.len == 0) return null;
        // Reject whitespace
        let wi: usize = 0;
        while (wi < raw.len) {
            const c = raw[wi];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') return null;
            wi += 1;
        }

        // 1. Optional @digest suffix
        let rest = raw;
        let digest: ([]const u8)? = null;
        let at_pos: usize? = null;
        let ai: usize = 0;
        while (ai < rest.len) {
            if (rest[ai] == '@') {
                at_pos = ai;
                break;
            }
            ai += 1;
        }
        if (ap := at_pos) {
            const dig = rest[ap + 1..];
            if (!isValidOciDigest(dig)) return null;
            digest = dig;
            rest = rest[0..ap];
        }
        if (rest.len == 0) return null;

        // 2. Optional :tag (colon MUST appear after the last '/')
        let last_slash: usize? = null;
        let last_colon: usize? = null;
        let si: usize = 0;
        while (si < rest.len) {
            if (rest[si] == '/') last_slash = si;
            if (rest[si] == ':') last_colon = si;
            si += 1;
        }

        let tag: ([]const u8)? = null;
        if (lc := last_colon) {
            let is_tag_colon = true;
            if (ls := last_slash) {
                if (lc < ls) is_tag_colon = false; // colon belongs to registry:port
            }
            if (is_tag_colon) {
                const t = rest[lc + 1..];
                if (!isValidTag(t)) return null;
                tag = t;
                rest = rest[0..lc];
            }
        }
        if (rest.len == 0) return null;

        // 3. Optional registry prefix (first segment contains '.' or ':' or equals "localhost")
        let first_slash: usize? = null;
        let fi: usize = 0;
        while (fi < rest.len) {
            if (rest[fi] == '/') {
                first_slash = fi;
                break;
            }
            fi += 1;
        }

        let registry: ([]const u8)? = null;
        let repo = rest;
        if (fs := first_slash) {
            const first_seg = rest[0..fs];
            if (first_seg.len == 0) return null;
            if (containsByte(first_seg, '.') || containsByte(first_seg, ':') || strEq(first_seg, "localhost")) {
                registry = first_seg;
                repo = rest[fs + 1..];
            }
        }
        if (repo.len == 0 || repo[0] == '/' || repo[repo.len - 1] == '/') return null;

        return DockerImageRef {
            registry: registry,
            repository: repo,
            tag: tag,
            digest: digest,
        };
    }

    pub fn effectiveRegistry(): []const u8 {
        return _.registry ?? "docker.io";
    }

    /// Effective tag (`"latest"` when neither tag nor digest is specified).
    pub fn effectiveTag(): ([]const u8)? {
        if (t := _.tag) return t;
        if (_.digest != null) return null;
        return "latest";
    }

    pub fn isOfficialDockerHub(): bool {
        return _.registry == null && !containsByte(_.repository, '/');
    }
}
