// @cap/otel-baggage — Zero-allocation W3C Baggage (OpenTelemetry) Header Parser & Serializer
// Clean-room MIT implementation in pure CP.

pub struct BaggageMember {
    pub key: string,
    pub value: string,
    pub properties: string,
}

fn isSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

fn isTchar(b: u8) -> bool {
    if (b >= 48 && b <= 57) { return true; }  // 0-9
    if (b >= 65 && b <= 90) { return true; }  // A-Z
    if (b >= 97 && b <= 122) { return true; } // a-z
    // ! # $ % & ' * + - . ^ _ ` | ~
    if (b == 33 || b == 35 || b == 36 || b == 37 || b == 38 || b == 39 || b == 42) { return true; }
    if (b == 43 || b == 45 || b == 46 || b == 94 || b == 95 || b == 96 || b == 124 || b == 126) { return true; }
    return false;
}

/// Validates a W3C Baggage key against the RFC 7230 `token` grammar (non-empty `1*tchar`).
pub fn isValidKey(key: string) -> bool {
    if (key.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < key.len) {
        if (!isTchar(key[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimSlice(s: string) -> string {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && isSpace(s[a])) {
        a += 1;
    }
    while (b > a && isSpace(s[b - 1])) {
        b -= 1;
    }
    return s[a..b];
}

fn strEq(a: string, b: string) -> bool {
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

fn parseSingleMember(seg: string, out: *BaggageMember) -> bool {
    const trimmed = trimSlice(seg);
    if (trimmed.len == 0) {
        return false;
    }

    // Split optional ";properties"
    let semi: usize = 0;
    while (semi < trimmed.len && trimmed[semi] != 59) {
        semi += 1;
    }
    const kv_part = trimmed[0..semi];
    let props: string = "";
    if (semi < trimmed.len) {
        props = trimSlice(trimmed[semi + 1..trimmed.len]);
    }

    // Split "key=value"
    let eq: usize = 0;
    while (eq < kv_part.len && kv_part[eq] != 61) {
        eq += 1;
    }
    if (eq >= kv_part.len) {
        return false;
    }
    const key = trimSlice(kv_part[0..eq]);
    const val = trimSlice(kv_part[eq + 1..kv_part.len]);
    if (!isValidKey(key)) {
        return false;
    }

    out.key = key;
    out.value = val;
    out.properties = props;
    return true;
}

/// Parses a comma-separated W3C `baggage` header into `out`.
/// Trims whitespace, splits optional `;properties`, skips malformed entries, and returns count written.
pub fn parseBaggage(header: string, out: []BaggageMember) -> usize {
    let pos: usize = 0;
    const n = header.len;
    let count: usize = 0;

    while (pos < n && count < out.len) {
        const start = pos;
        while (pos < n && header[pos] != 44) {
            pos += 1;
        }
        let m = BaggageMember { key: "", value: "", properties: "" };
        if (parseSingleMember(header[start..pos], &m)) {
            out[count] = m;
            count += 1;
        }
        if (pos < n && header[pos] == 44) {
            pos += 1;
        }
    }

    return count;
}

/// Counts the number of valid members in a W3C `baggage` header.
pub fn countMembers(header: string) -> usize {
    let pos: usize = 0;
    const n = header.len;
    let count: usize = 0;

    while (pos < n) {
        const start = pos;
        while (pos < n && header[pos] != 44) {
            pos += 1;
        }
        let m = BaggageMember { key: "", value: "", properties: "" };
        if (parseSingleMember(header[start..pos], &m)) {
            count += 1;
        }
        if (pos < n && header[pos] == 44) {
            pos += 1;
        }
    }

    return count;
}

fn hexVal(b: u8) -> i32 {
    if (b >= 48 && b <= 57) { return (b - 48) as i32; }
    if (b >= 65 && b <= 70) { return (b - 55) as i32; }
    if (b >= 97 && b <= 102) { return (b - 87) as i32; }
    return -1;
}

fn percentDecode(src: string, out: []u8, out_len: *usize) -> bool {
    let i: usize = 0;
    let w: usize = 0;
    const n = src.len;

    while (i < n) {
        if (w >= out.len) {
            return false;
        }
        if (src[i] == 37 && i + 2 < n) {
            const hi = hexVal(src[i + 1]);
            const lo = hexVal(src[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out[w] = ((hi << 4) | lo) as u8;
                w += 1;
                i += 3;
                continue;
            }
        }
        out[w] = src[i];
        w += 1;
        i += 1;
    }

    out_len.* = w;
    return true;
}

/// Looks up `target_key` in `header` (last occurrence wins per W3C Baggage),
/// percent-decodes its value into `out`, and stores the decoded length in `out_len`.
/// Returns true if found and decoded cleanly.
pub fn getDecodedValue(header: string, target_key: string, out: []u8, out_len: *usize) -> bool {
    if (target_key.len == 0) {
        return false;
    }
    let found = false;
    let found_val: string = "";

    let pos: usize = 0;
    const n = header.len;
    while (pos < n) {
        const start = pos;
        while (pos < n && header[pos] != 44) {
            pos += 1;
        }
        let m = BaggageMember { key: "", value: "", properties: "" };
        if (parseSingleMember(header[start..pos], &m)) {
            if (strEq(m.key, target_key)) {
                found = true;
                found_val = m.value;
            }
        }
        if (pos < n && header[pos] == 44) {
            pos += 1;
        }
    }

    if (!found) {
        return false;
    }
    return percentDecode(found_val, out, out_len);
}

fn hexUpper(nibble: u8) -> u8 {
    if (nibble < 10) {
        return 48 + nibble;
    }
    return 55 + nibble;
}

fn needsPercentEncode(b: u8) -> bool {
    // Encode controls, whitespace, ", ,, ;, =, \, %, and non-ASCII
    if (b <= 32 || b >= 127) { return true; }
    return b == 34 || b == 37 || b == 44 || b == 59 || b == 61 || b == 92;
}

/// Serializes `members` into a standard W3C `baggage` header value in `out`.
/// Automatically percent-encodes `,`, `;`, `=`, `%`, whitespace, and non-ASCII bytes in values,
/// and preserves `;properties`. Returns bytes written, or `0` on invalid key or buffer overflow.
pub fn formatBaggage(members: []const BaggageMember, out: []u8) -> usize {
    if (members.len == 0) {
        return 0;
    }
    let w: usize = 0;
    let mi: usize = 0;
    while (mi < members.len) {
        const m = members[mi];
        if (!isValidKey(m.key)) {
            return 0;
        }
        if (mi > 0) {
            if (w >= out.len) { return 0; }
            out[w] = 44; // ','
            w += 1;
        }

        // Write key
        if (w + m.key.len + 1 > out.len) { return 0; }
        let ki: usize = 0;
        while (ki < m.key.len) {
            out[w] = m.key[ki];
            w += 1;
            ki += 1;
        }
        out[w] = 61; // '='
        w += 1;

        // Write percent-encoded value
        let vi: usize = 0;
        while (vi < m.value.len) {
            const b = m.value[vi];
            if (needsPercentEncode(b)) {
                if (w + 3 > out.len) { return 0; }
                out[w] = 37;
                out[w + 1] = hexUpper((b >> 4) & 0x0F);
                out[w + 2] = hexUpper(b & 0x0F);
                w += 3;
            } else {
                if (w >= out.len) { return 0; }
                out[w] = b;
                w += 1;
            }
            vi += 1;
        }

        // Optional ";properties"
        if (m.properties.len > 0) {
            if (w + 1 + m.properties.len > out.len) { return 0; }
            out[w] = 59; // ';'
            w += 1;
            let pi: usize = 0;
            while (pi < m.properties.len) {
                out[w] = m.properties[pi];
                w += 1;
                pi += 1;
            }
        }

        mi += 1;
    }

    return w;
}
