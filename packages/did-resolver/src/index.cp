// @cap/did-parse — Zero-heap W3C DID Core 1.0 parser, validator & formatter
// MIT License (clean-room CP implementation)

import { strEq, isHexDigit, isAlnum, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub struct DidUrl {
    pub did: string,
    pub method: string,
    pub id: string,
    pub path: string,
    pub query: string,
    pub fragment: string,
    pub valid: bool,
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
            if (i + 2 >= id.len || !isHexDigit(id[i + 1]) || !isHexDigit(id[i + 2])) {
                return false;
            }
            i += 3;
            continue;
        }
        const ok: bool = isAlnum(c)
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

    if (input.len < 7 || !strEq(input[0..4], "did:")) {
        return fail;
    }

    // Extract optional #fragment
    const frag_pos: usize = findByte(input, '#' as u8, 4) ?? input.len;
    let fragment: string = "";
    if (frag_pos < input.len) {
        fragment = input[(frag_pos + 1)..input.len];
    }

    // Extract optional ?query before #fragment
    const query_pos: usize = findByte(input[0..frag_pos], '?' as u8, 4) ?? frag_pos;
    let query: string = "";
    if (query_pos < frag_pos) {
        query = input[(query_pos + 1)..frag_pos];
    }

    // Extract optional /path before ?query
    const path_pos: usize = findByte(input[0..query_pos], '/' as u8, 4) ?? query_pos;
    let path: string = "";
    if (path_pos < query_pos) {
        path = input[path_pos..query_pos];
    }

    // Bare DID is input[0..path_pos]
    const c2: usize = findByte(input[0..path_pos], ':' as u8, 4) ?? path_pos;
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
    return writeStr(s, out, pos) as i64;
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
