// packages/@cap/docker-ref/src/index.cp
// Zero-allocation OCI / Docker image reference parser, validator & normalizer.
// MIT clean-room pure CP implementation.

/// Zero-copy parsed OCI / Docker image reference.
pub struct ImageRef {
    pub registry: string,
    pub repository: string,
    pub tag: string,
    pub digest: string,
    pub valid: bool,
}

fn drIsLower(c: u8): bool {
    return c >= ('a' as u8) && c <= ('z' as u8);
}

fn drIsUpper(c: u8): bool {
    return c >= ('A' as u8) && c <= ('Z' as u8);
}

fn drIsDigit(c: u8): bool {
    return c >= ('0' as u8) && c <= ('9' as u8);
}

fn drIsLowerHex(c: u8): bool {
    return drIsDigit(c) || (c >= ('a' as u8) && c <= ('f' as u8));
}

fn drEqBytes(a: []const u8, b: []const u8): bool {
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

fn drContainsByte(s: []const u8, target: u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == target) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Validate an OCI image tag: length 1..128, starting with `[a-zA-Z0-9_]`,
/// followed by `[a-zA-Z0-9_.-]`.
pub fn isValidTag(tag: string): bool {
    if (tag.len == 0 || tag.len > 128) {
        return false;
    }
    const c0 = tag[0];
    if (!drIsLower(c0) && !drIsUpper(c0) && !drIsDigit(c0) && c0 != ('_' as u8)) {
        return false;
    }
    let i: usize = 1;
    while (i < tag.len) {
        const c = tag[i];
        const ok = drIsLower(c) || drIsUpper(c) || drIsDigit(c) || c == ('_' as u8) || c == ('.' as u8) || c == ('-' as u8);
        if (!ok) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Validate an OCI content digest: `sha256:<64-lowercase-hex>` or `sha512:<128-lowercase-hex>`.
pub fn isValidDigest(digest: string): bool {
    let hexLen: usize = 0;
    if (digest.len == 71 && drEqBytes(digest[0..7], "sha256:")) {
        hexLen = 64;
    } else if (digest.len == 135 && drEqBytes(digest[0..7], "sha512:")) {
        hexLen = 128;
    } else {
        return false;
    }

    let i: usize = 0;
    while (i < hexLen) {
        if (!drIsLowerHex(digest[7 + i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn drIsValidRegistry(reg: []const u8): bool {
    if (reg.len == 0 || reg.len > 255) {
        return false;
    }
    let i: usize = 0;
    while (i < reg.len) {
        const c = reg[i];
        const ok = drIsLower(c) || drIsUpper(c) || drIsDigit(c) || c == ('.' as u8) || c == ('-' as u8) || c == (':' as u8);
        if (!ok) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Validate an OCI repository path: slash-separated lowercase components `[a-z0-9][a-z0-9._-]*[a-z0-9]?`.
fn drIsValidRepository(repo: []const u8): bool {
    if (repo.len == 0 || repo.len > 255) {
        return false;
    }
    let pos: usize = 0;
    while (pos < repo.len) {
        let end = pos;
        while (end < repo.len && repo[end] != ('/' as u8)) {
            end += 1;
        }
        const segLen = end - pos;
        if (segLen == 0) {
            return false;
        }
        const first = repo[pos];
        const last = repo[end - 1];
        if ((!drIsLower(first) && !drIsDigit(first)) || (!drIsLower(last) && !drIsDigit(last))) {
            return false;
        }
        let i = pos;
        while (i < end) {
            const c = repo[i];
            const ok = drIsLower(c) || drIsDigit(c) || c == ('.' as u8) || c == ('_' as u8) || c == ('-' as u8);
            if (!ok) {
                return false;
            }
            i += 1;
        }
        if (end < repo.len) {
            if (end + 1 == repo.len) {
                // Trailing slash is invalid.
                return false;
            }
            pos = end + 1;
        } else {
            pos = repo.len;
        }
    }
    return true;
}

/// Parse an OCI / Docker image reference `[registry/][namespace/]name[:tag][@digest]`.
/// Distinguishes registry port colons (e.g. `localhost:5000/app:v1`) from `:tag`.
/// Recognizes first path segments containing `.`, `:`, or `localhost` as `registry`.
pub fn parseImageRef(input: string): ImageRef {
    let invalid = ImageRef{
        registry: "",
        repository: "",
        tag: "",
        digest: "",
        valid: false,
    };

    if (input.len == 0) {
        return invalid;
    }

    // 1. Split optional '@digest'.
    let atIdx = input.len;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == ('@' as u8)) {
            atIdx = i;
            break;
        }
        i += 1;
    }

    let digestSlice: []const u8 = "";
    let nameTag = input;
    if (atIdx < input.len) {
        digestSlice = input[(atIdx + 1)..input.len];
        if (!isValidDigest(digestSlice)) {
            return invalid;
        }
        nameTag = input[0..atIdx];
    }
    if (nameTag.len == 0) {
        return invalid;
    }

    // 2. Split optional ':tag' (only if ':' appears after the last '/').
    let hasSlash = false;
    let lastSlash: usize = 0;
    let hasColon = false;
    let lastColon: usize = 0;
    let j: usize = 0;
    while (j < nameTag.len) {
        if (nameTag[j] == ('/' as u8)) {
            hasSlash = true;
            lastSlash = j;
        } else if (nameTag[j] == (':' as u8)) {
            hasColon = true;
            lastColon = j;
        }
        j += 1;
    }

    let tagSlice: []const u8 = "";
    let fullName = nameTag;
    if (hasColon && (!hasSlash || lastColon > lastSlash)) {
        tagSlice = nameTag[(lastColon + 1)..nameTag.len];
        if (!isValidTag(tagSlice)) {
            return invalid;
        }
        fullName = nameTag[0..lastColon];
    }
    if (fullName.len == 0) {
        return invalid;
    }

    // 3. Split optional 'registry/' if first segment contains '.', ':', or equals "localhost".
    let firstSlash = fullName.len;
    let k: usize = 0;
    while (k < fullName.len) {
        if (fullName[k] == ('/' as u8)) {
            firstSlash = k;
            break;
        }
        k += 1;
    }

    let regSlice: []const u8 = "";
    let repoSlice = fullName;
    if (firstSlash < fullName.len) {
        const firstSeg = fullName[0..firstSlash];
        const looksLikeReg = drContainsByte(firstSeg, '.' as u8) || drContainsByte(firstSeg, ':' as u8) || drEqBytes(firstSeg, "localhost");
        if (looksLikeReg) {
            if (!drIsValidRegistry(firstSeg)) {
                return invalid;
            }
            regSlice = firstSeg;
            repoSlice = fullName[(firstSlash + 1)..fullName.len];
        }
    }

    // 4. Validate repository path.
    if (!drIsValidRepository(repoSlice)) {
        return invalid;
    }

    return ImageRef{
        registry: regSlice,
        repository: repoSlice,
        tag: tagSlice,
        digest: digestSlice,
        valid: true,
    };
}

fn drWrite(out: []u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.*] = src[i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

/// Format a valid `ImageRef` in canonical normalized form into `out`:
/// - Empty `registry` defaults to `docker.io`.
/// - Single-segment repositories on `docker.io` are prefixed with `library/`.
/// - When both `tag` and `digest` are empty, `:latest` is appended.
/// Returns bytes written, or `0` on invalid input or insufficient `out` buffer.
pub fn formatNormalized(ref_in: ImageRef, out: []u8): usize {
    if (!ref_in.valid || ref_in.repository.len == 0) {
        return 0;
    }

    let reg = ref_in.registry;
    let isDockerHub = false;
    if (reg.len == 0) {
        reg = "docker.io";
        isDockerHub = true;
    } else if (drEqBytes(reg, "docker.io")) {
        isDockerHub = true;
    }

    let pos: usize = 0;
    if (!drWrite(out, &pos, reg)) {
        return 0;
    }
    if (!drWrite(out, &pos, "/")) {
        return 0;
    }
    if (isDockerHub && !drContainsByte(ref_in.repository, '/' as u8)) {
        if (!drWrite(out, &pos, "library/")) {
            return 0;
        }
    }
    if (!drWrite(out, &pos, ref_in.repository)) {
        return 0;
    }

    if (ref_in.tag.len > 0) {
        if (!drWrite(out, &pos, ":")) {
            return 0;
        }
        if (!drWrite(out, &pos, ref_in.tag)) {
            return 0;
        }
    } else if (ref_in.digest.len == 0) {
        if (!drWrite(out, &pos, ":latest")) {
            return 0;
        }
    }

    if (ref_in.digest.len > 0) {
        if (!drWrite(out, &pos, "@")) {
            return 0;
        }
        if (!drWrite(out, &pos, ref_in.digest)) {
            return 0;
        }
    }

    return pos;
}

export {
    ImageRef,
    isValidTag,
    isValidDigest,
    parseImageRef,
    formatNormalized,
};
