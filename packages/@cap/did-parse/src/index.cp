// @cap/did-parse — Zero-heap W3C DID Core 1.0 parser, validator & formatter
// MIT License (clean-room CP implementation)

pub struct DidUrl {
    pub did: string,
    pub method: string,
    pub id: string,
    pub path: string,
    pub query: string,
    pub fragment: string,
    pub valid: bool,
}

fn bytesEq(a: string, b: string) -> bool {
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

fn isHex(c: u8) -> bool {
    return (c >= '0' as u8 && c <= '9' as u8)
        || (c >= 'a' as u8 && c <= 'f' as u8)
        || (c >= 'A' as u8 && c <= 'F' as u8);
}

pub fn isValidMethod(method: string) -> bool {
    if (method.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < method.len) {
        const c: u8 = method[i];
        const ok: bool = (c >= 'a' as u8 && c <= 'z' as u8)
            || (c >= '0' as u8 && c <= '9' as u8);
        if (!ok) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn isValidMethodId(id: string) -> bool {
    if (id.len == 0) {
        return false;
    }
    if (id[id.len - 1] == ':' as u8) {
        return false;
    }
    let i: usize = 0;
    while (i < id.len) {
        const c: u8 = id[i];
        if (c == '%' as u8) {
            if (i + 2 >= id.len || !isHex(id[i + 1]) || !isHex(id[i + 2])) {
                return false;
            }
            i += 3;
            continue;
        }
        const ok: bool = (c >= 'a' as u8 && c <= 'z' as u8)
            || (c >= 'A' as u8 && c <= 'Z' as u8)
            || (c >= '0' as u8 && c <= '9' as u8)
            || c == '.' as u8
            || c == '-' as u8
            || c == '_' as u8
            || c == ':' as u8;
        if (!ok) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn parseDid(input: string) -> DidUrl {
    const fail = DidUrl {
        did: "",
        method: "",
        id: "",
        path: "",
        query: "",
        fragment: "",
        valid: false,
    };

    if (input.len < 7 || !bytesEq(input[0..4], "did:")) {
        return fail;
    }

    // Extract optional #fragment
    let frag_pos: usize = input.len;
    let i: usize = 4;
    while (i < input.len) {
        if (input[i] == '#' as u8) {
            frag_pos = i;
            break;
        }
        i += 1;
    }
    let fragment: string = "";
    if (frag_pos < input.len) {
        fragment = input[(frag_pos + 1)..input.len];
    }

    // Extract optional ?query before #fragment
    let query_pos: usize = frag_pos;
    let iq: usize = 4;
    while (iq < frag_pos) {
        if (input[iq] == '?' as u8) {
            query_pos = iq;
            break;
        }
        iq += 1;
    }
    let query: string = "";
    if (query_pos < frag_pos) {
        query = input[(query_pos + 1)..frag_pos];
    }

    // Extract optional /path before ?query
    let path_pos: usize = query_pos;
    let ip: usize = 4;
    while (ip < query_pos) {
        if (input[ip] == '/' as u8) {
            path_pos = ip;
            break;
        }
        ip += 1;
    }
    let path: string = "";
    if (path_pos < query_pos) {
        path = input[path_pos..query_pos];
    }

    // Bare DID is input[0..path_pos]
    let c2: usize = 4;
    while (c2 < path_pos && input[c2] != ':' as u8) {
        c2 += 1;
    }
    if (c2 >= path_pos) {
        return fail;
    }

    const method: string = input[4..c2];
    const id: string = input[(c2 + 1)..path_pos];
    if (!isValidMethod(method) || !isValidMethodId(id)) {
        return fail;
    }

    return DidUrl {
        did: input[0..path_pos],
        method: method,
        id: id,
        path: path,
        query: query,
        fragment: fragment,
        valid: true,
    };
}

fn appendSlice(out: []u8, pos: usize, s: string) -> i64 {
    if (s.len > out.len - pos) {
        return -1;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos + i] = s[i];
        i += 1;
    }
    return (pos + s.len) as i64;
}

pub fn formatDid(method: string, id: string, fragment: string, out: []u8) -> usize {
    if (!isValidMethod(method) || !isValidMethodId(id)) {
        return 0;
    }
    let p: i64 = appendSlice(out, 0, "did:");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, method);
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, ":");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, id);
    if (p < 0) {
        return 0;
    }
    if (fragment.len > 0) {
        p = appendSlice(out, p as usize, "#");
        if (p < 0) {
            return 0;
        }
        p = appendSlice(out, p as usize, fragment);
        if (p < 0) {
            return 0;
        }
    }
    return p as usize;
}
