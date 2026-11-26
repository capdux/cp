// @cap/oci-ref — Zero-heap OCI / Docker container image reference parser & canonicalizer
// Clean-room MIT implementation in pure CP (replaces npm `docker-reference` / Rust `oci-distribution` Reference).

pub struct OciRef {
    pub valid: bool,
    pub registry: string,
    pub repository: string,
    pub tag: string,
    pub digest: string,
    pub is_official_library: bool,
}

fn findByte(s: string, ch: u8) -> usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) { return i; }
        i += 1;
    }
    return null;
}

fn findLastByte(s: string, ch: u8) -> usize? {
    let i: usize = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == ch) { return i; }
    }
    return null;
}

fn isValidDigest(d: string) -> bool {
    // Expects "<algo>:<hex>", e.g. "sha256:<64 hex chars>" or "sha512:<128 hex chars>"
    const colonOpt = findByte(d, ':');
    if (colonOpt == null) { return false; }
    const colon: usize = colonOpt ?? (0 as usize);
    if (colon == 0 || colon + 1 >= d.len) { return false; }
    const algo: string = d[0..colon];
    const hex: string = d[(colon + 1)..d.len];
    if (algo == "sha256" && hex.len != 64) { return false; }
    if (algo == "sha512" && hex.len != 128) { return false; }
    if (hex.len < 32) { return false; }
    let i: usize = 0;
    while (i < hex.len) {
        const c: u8 = hex[i];
        const isHex: bool = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!isHex) { return false; }
        i += 1;
    }
    return true;
}

fn isValidTag(t: string) -> bool {
    if (t.len == 0 || t.len > 128) { return false; }
    if (t[0] == '.' || t[0] == '-') { return false; }
    let i: usize = 0;
    while (i < t.len) {
        const c: u8 = t[i];
        const ok: bool = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-';
        if (!ok) { return false; }
        i += 1;
    }
    return true;
}

fn isValidRepoPath(p: string) -> bool {
    if (p.len == 0 || p.len > 255) { return false; }
    if (p[0] == '/' || p[p.len - 1] == '/') { return false; }
    let i: usize = 0;
    let prevSlash: bool = false;
    while (i < p.len) {
        const c: u8 = p[i];
        if (c == '/') {
            if (prevSlash) { return false; }
            prevSlash = true;
        } else {
            prevSlash = false;
            const ok: bool = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                c == '_' || c == '.' || c == '-';
            if (!ok) { return false; }
        }
        i += 1;
    }
    return true;
}

fn isRegistryDomain(seg: string) -> bool {
    if (seg == "localhost") { return true; }
    let i: usize = 0;
    while (i < seg.len) {
        if (seg[i] == '.' || seg[i] == ':') { return true; }
        i += 1;
    }
    return false;
}

/// Parse an OCI / Docker image reference into registry, repository, tag, and digest.
/// Follows Docker Distribution normalization rules:
/// - Default registry is `"docker.io"`
/// - Bare image names (e.g. `"nginx"`) get `is_official_library = true`
/// - When neither tag nor digest is specified, tag defaults to `"latest"`
pub fn parseOciRef(raw: string) -> OciRef {
    const invalid: OciRef = OciRef {
        valid: false,
        registry: "",
        repository: "",
        tag: "",
        digest: "",
        is_official_library: false,
    };

    if (raw.len == 0) { return invalid; }

    // 1. Split optional @digest
    let nameAndTag: string = raw;
    let digest: string = "";
    const atOpt = findByte(raw, '@');
    if (atOpt != null) {
        const atPos: usize = atOpt ?? (0 as usize);
        nameAndTag = raw[0..atPos];
        digest = raw[(atPos + 1)..raw.len];
        if (!isValidDigest(digest)) { return invalid; }
    }

    if (nameAndTag.len == 0) { return invalid; }

    // 2. Split optional :tag (must appear AFTER the last '/')
    let namePart: string = nameAndTag;
    let tag: string = "";
    const lastSlashOpt = findLastByte(nameAndTag, '/');
    const lastColonOpt = findLastByte(nameAndTag, ':');
    if (lastColonOpt != null) {
        const colonPos: usize = lastColonOpt ?? (0 as usize);
        const hasSlash: bool = lastSlashOpt != null;
        const slashPos: usize = lastSlashOpt ?? (0 as usize);
        if (!hasSlash || colonPos > slashPos) {
            namePart = nameAndTag[0..colonPos];
            tag = nameAndTag[(colonPos + 1)..nameAndTag.len];
            if (!isValidTag(tag)) { return invalid; }
        }
    }

    if (tag.len == 0 && digest.len == 0) {
        tag = "latest";
    }

    // 3. Split registry vs repository
    let registry: string = "docker.io";
    let repo: string = namePart;
    let isOfficial: bool = false;

    const firstSlashOpt = findByte(namePart, '/');
    if (firstSlashOpt == null) {
        // Bare name like "nginx" or "alpine"
        isOfficial = true;
    } else {
        const slashPos: usize = firstSlashOpt ?? (0 as usize);
        const firstSeg: string = namePart[0..slashPos];
        const rest: string = namePart[(slashPos + 1)..namePart.len];
        if (isRegistryDomain(firstSeg)) {
            registry = firstSeg;
            repo = rest;
        } else {
            if (firstSeg == "library") {
                isOfficial = true;
            }
            repo = namePart;
        }
    }

    if (!isValidRepoPath(repo)) { return invalid; }

    return OciRef {
        valid: true,
        registry: registry,
        repository: repo,
        tag: tag,
        digest: digest,
        is_official_library: isOfficial,
    };
}

/// Format a parsed OCI reference into canonical form (`registry/repository[:tag][@digest]`).
/// Official Docker Hub bare images are normalized to `docker.io/library/<name>`.
pub fn formatCanonical(r: OciRef, out: []u8) -> string {
    if (!r.valid) { return out[0..0]; }

    let pos: usize = 0;
    const parts0: string = r.registry;
    let i: usize = 0;
    while (i < parts0.len && pos < out.len) { out[pos] = parts0[i]; pos += 1; i += 1; }
    if (pos < out.len) { out[pos] = '/'; pos += 1; }

    // Prepend "library/" if bare official Docker Hub image
    if (r.is_official_library && findByte(r.repository, '/') == null) {
        const libPrefix: string = "library/";
        i = 0;
        while (i < libPrefix.len && pos < out.len) { out[pos] = libPrefix[i]; pos += 1; i += 1; }
    }

    i = 0;
    while (i < r.repository.len && pos < out.len) { out[pos] = r.repository[i]; pos += 1; i += 1; }

    if (r.tag.len > 0) {
        if (pos < out.len) { out[pos] = ':'; pos += 1; }
        i = 0;
        while (i < r.tag.len && pos < out.len) { out[pos] = r.tag[i]; pos += 1; i += 1; }
    }

    if (r.digest.len > 0) {
        if (pos < out.len) { out[pos] = '@'; pos += 1; }
        i = 0;
        while (i < r.digest.len && pos < out.len) { out[pos] = r.digest[i]; pos += 1; i += 1; }
    }

    return out[0..pos];
}

/// Returns true if the reference is pinned by an immutable cryptographic digest.
pub fn isPinned(r: OciRef) -> bool {
    return r.valid && r.digest.len > 0;
}
