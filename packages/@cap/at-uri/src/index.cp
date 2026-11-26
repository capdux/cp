// @cap/at-uri — Zero-allocation AT Protocol / Bluesky at:// URI, NSID & Record Key Utilities
// Clean-room MIT implementation in pure CP.

pub struct AtUri {
    pub authority: string,
    pub collection: string,
    pub rkey: string,
    pub is_did: bool,
    pub valid: bool,
}

fn invalidAtUri() -> AtUri {
    return AtUri {
        authority: "",
        collection: "",
        rkey: "",
        is_did: false,
        valid: false,
    };
}

fn isLowerAlpha(b: u8) -> bool {
    return b >= 97 && b <= 122;
}

fn isAlpha(b: u8) -> bool {
    return (b >= 65 && b <= 90) || (b >= 97 && b <= 122);
}

fn isDigit(b: u8) -> bool {
    return b >= 48 && b <= 57;
}

fn isAlphaNum(b: u8) -> bool {
    return isAlpha(b) || isDigit(b);
}

/// Validates an AT Protocol NSID (Namespaced Identifier), e.g. `app.bsky.feed.post`.
/// Requires >= 3 dot-separated segments, lowercase reversed domain authority segments,
/// alphanumeric camelCase-friendly final name segment starting with a letter, and total length <= 317.
pub fn isValidNsid(nsid: string) -> bool {
    if (nsid.len < 5 || nsid.len > 317) {
        return false;
    }
    if (nsid[0] == 46 || nsid[nsid.len - 1] == 46) {
        return false;
    }

    // Find last dot to separate authority prefix from name segment
    let last_dot: usize = nsid.len;
    let scan = nsid.len;
    while (scan > 0) {
        scan -= 1;
        if (nsid[scan] == 46) {
            last_dot = scan;
            break;
        }
    }
    if (last_dot == nsid.len || last_dot == 0) {
        return false;
    }

    // Validate final name segment
    const name_start = last_dot + 1;
    const name_len = nsid.len - name_start;
    if (name_len == 0 || name_len > 63 || !isAlpha(nsid[name_start])) {
        return false;
    }
    let ni = name_start + 1;
    while (ni < nsid.len) {
        if (!isAlphaNum(nsid[ni])) {
            return false;
        }
        ni += 1;
    }

    // Validate authority segments (at least 2 segments before last_dot)
    let seg_count: usize = 0;
    let pos: usize = 0;
    while (pos < last_dot) {
        const s_start = pos;
        while (pos < last_dot && nsid[pos] != 46) {
            pos += 1;
        }
        const s_end = pos;
        const s_len = s_end - s_start;
        if (s_len == 0 || s_len > 63) {
            return false;
        }
        // First segment (reversed TLD) cannot start with a digit
        if (seg_count == 0 && !isLowerAlpha(nsid[s_start])) {
            return false;
        }
        if (nsid[s_start] == 45 || nsid[s_end - 1] == 45) {
            return false;
        }
        let k = s_start;
        while (k < s_end) {
            const ch = nsid[k];
            if (!isLowerAlpha(ch) && !isDigit(ch) && ch != 45) {
                return false;
            }
            k += 1;
        }
        seg_count += 1;
        if (pos < last_dot && nsid[pos] == 46) {
            pos += 1;
            if (pos == last_dot) {
                return false; // empty segment from ".."
            }
        }
    }

    return seg_count >= 2;
}

/// Validates an AT Protocol Record Key (`rkey`):
/// length 1..512, not `"."` or `".."`, allowed chars `A-Za-z0-9._~:-`.
pub fn isValidRkey(rkey: string) -> bool {
    if (rkey.len == 0 || rkey.len > 512) {
        return false;
    }
    if (rkey.len == 1 && rkey[0] == 46) {
        return false; // "."
    }
    if (rkey.len == 2 && rkey[0] == 46 && rkey[1] == 46) {
        return false; // ".."
    }
    let i: usize = 0;
    while (i < rkey.len) {
        const ch = rkey[i];
        const ok = isAlphaNum(ch) || ch == 46 || ch == 95 || ch == 126 || ch == 58 || ch == 45;
        if (!ok) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isValidDid(s: string) -> bool {
    // did:<method>:<id>
    if (s.len < 7) {
        return false;
    }
    if (s[0] != 100 || s[1] != 105 || s[2] != 100 || s[3] != 58) {
        return false;
    }
    let pos: usize = 4;
    const m_start = pos;
    while (pos < s.len && s[pos] != 58) {
        if (!isLowerAlpha(s[pos]) && !isDigit(s[pos])) {
            return false;
        }
        pos += 1;
    }
    if (pos == m_start || pos + 1 >= s.len) {
        return false;
    }
    pos += 1; // skip ':'
    while (pos < s.len) {
        const ch = s[pos];
        const ok = isAlphaNum(ch) || ch == 46 || ch == 95 || ch == 58 || ch == 37 || ch == 45;
        if (!ok) {
            return false;
        }
        pos += 1;
    }
    return true;
}

fn isValidHandle(s: string) -> bool {
    if (s.len < 3 || s.len > 253 || s[0] == 46 || s[s.len - 1] == 46) {
        return false;
    }
    let seg_count: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        const start = pos;
        while (pos < s.len && s[pos] != 46) {
            pos += 1;
        }
        const end = pos;
        const slen = end - start;
        if (slen == 0 || slen > 63 || s[start] == 45 || s[end - 1] == 45) {
            return false;
        }
        let k = start;
        while (k < end) {
            const ch = s[k];
            if (!isAlphaNum(ch) && ch != 45) {
                return false;
            }
            k += 1;
        }
        seg_count += 1;
        if (pos < s.len && s[pos] == 46) {
            pos += 1;
        }
    }
    return seg_count >= 2;
}

/// Parses an AT Protocol URI (`at://<authority>[/<collection>[/<rkey>]]`) with zero heap allocation.
/// Validates `at://` prefix, DID/handle authority, optional NSID collection, and optional rkey.
pub fn parseAtUri(uri: string) -> AtUri {
    if (uri.len < 6) {
        return invalidAtUri();
    }
    // Strict lowercase "at://"
    if (uri[0] != 97 || uri[1] != 116 || uri[2] != 58 || uri[3] != 47 || uri[4] != 47) {
        return invalidAtUri();
    }
    if (uri[uri.len - 1] == 47) {
        // Reject trailing slash
        return invalidAtUri();
    }

    const rest = uri[5..uri.len];
    let s1: usize = rest.len;
    let s2: usize = rest.len;
    let i: usize = 0;
    while (i < rest.len) {
        if (rest[i] == 47) {
            if (s1 == rest.len) {
                s1 = i;
            } else if (s2 == rest.len) {
                s2 = i;
            } else {
                // Too many path segments (> 3)
                return invalidAtUri();
            }
        }
        i += 1;
    }

    const authority = rest[0..s1];
    let is_did = false;
    if (isValidDid(authority)) {
        is_did = true;
    } else if (!isValidHandle(authority)) {
        return invalidAtUri();
    }

    let collection: string = "";
    let rkey: string = "";

    if (s1 < rest.len) {
        if (s2 < rest.len) {
            collection = rest[s1 + 1..s2];
            rkey = rest[s2 + 1..rest.len];
            if (!isValidNsid(collection) || !isValidRkey(rkey)) {
                return invalidAtUri();
            }
        } else {
            collection = rest[s1 + 1..rest.len];
            if (!isValidNsid(collection)) {
                return invalidAtUri();
            }
        }
    }

    return AtUri {
        authority: authority,
        collection: collection,
        rkey: rkey,
        is_did: is_did,
        valid: true,
    };
}

fn pushSlice(dst: []u8, pos: usize, src: string) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return (pos + src.len) as i64;
}

/// Formats an AT Protocol URI `at://<authority>[/<collection>[/<rkey>]]` into `out`.
/// Returns bytes written, or `0` if invalid or `out` is too small.
pub fn formatAtUri(authority: string, collection: string, rkey: string, out: []u8) -> usize {
    if (authority.len == 0 || (rkey.len > 0 && collection.len == 0)) {
        return 0;
    }
    let p = pushSlice(out, 0, "at://");
    if (p < 0) { return 0; }
    p = pushSlice(out, p as usize, authority);
    if (p < 0) { return 0; }

    if (collection.len > 0) {
        p = pushSlice(out, p as usize, "/");
        if (p < 0) { return 0; }
        p = pushSlice(out, p as usize, collection);
        if (p < 0) { return 0; }
        if (rkey.len > 0) {
            p = pushSlice(out, p as usize, "/");
            if (p < 0) { return 0; }
            p = pushSlice(out, p as usize, rkey);
            if (p < 0) { return 0; }
        }
    }

    return p as usize;
}
