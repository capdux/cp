// @cap/multipart-boundary — Zero-heap RFC 7578 multipart/form-data parser
// MIT License (clean-room CP implementation)

import { eqIgnoreCase, strEq, isAlnum } from "cap:ascii";

pub struct MultipartPart {
    pub headers: string,
    pub body: string,
    pub name: string,
    pub filename: string,
    pub content_type: string,
    pub next_pos: usize,
    pub valid: bool,
}

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8;
}

fn isBchar(c: u8) -> bool {
    if (isAlnum(c)) {
        return true;
    }
    return c == '\'' as u8 || c == '(' as u8 || c == ')' as u8 || c == '+' as u8
        || c == '_' as u8 || c == ',' as u8 || c == '-' as u8 || c == '.' as u8
        || c == '/' as u8 || c == ':' as u8 || c == '=' as u8 || c == '?' as u8
        || c == ' ' as u8;
}

pub fn isValidBoundary(boundary: string) -> bool {
    if (boundary.len == 0 || boundary.len > 70) {
        return false;
    }
    if (boundary[boundary.len - 1] == ' ' as u8) {
        return false;
    }
    let i: usize = 0;
    while (i < boundary.len) {
        if (!isBchar(boundary[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn extractBoundary(content_type: string) -> string {
    let pos: usize = 0;
    while (pos < content_type.len && content_type[pos] != ';' as u8) {
        pos += 1;
    }
    if (pos >= content_type.len) {
        return "";
    }
    pos += 1;

    while (pos < content_type.len) {
        while (pos < content_type.len && isWs(content_type[pos])) {
            pos += 1;
        }
        let seg_end: usize = pos;
        let in_quote: bool = false;
        while (seg_end < content_type.len) {
            const c: u8 = content_type[seg_end];
            if (in_quote) {
                if (c == '\\' as u8 && seg_end + 1 < content_type.len) {
                    seg_end += 2;
                    continue;
                }
                if (c == '"' as u8) {
                    in_quote = false;
                }
            } else {
                if (c == '"' as u8) {
                    in_quote = true;
                } else if (c == ';' as u8) {
                    break;
                }
            }
            seg_end += 1;
        }

        let eq: usize = pos;
        while (eq < seg_end && content_type[eq] != '=' as u8) {
            eq += 1;
        }
        if (eq < seg_end) {
            let k_end: usize = eq;
            while (k_end > pos && isWs(content_type[k_end - 1])) {
                k_end -= 1;
            }
            if (eqIgnoreCase(content_type[pos..k_end], "boundary")) {
                let v_s: usize = eq + 1;
                let v_e: usize = seg_end;
                while (v_s < v_e && isWs(content_type[v_s])) {
                    v_s += 1;
                }
                while (v_e > v_s && isWs(content_type[v_e - 1])) {
                    v_e -= 1;
                }
                if (v_e - v_s >= 2 && content_type[v_s] == '"' as u8 && content_type[v_e - 1] == '"' as u8) {
                    v_s += 1;
                    v_e -= 1;
                }
                const cand: string = content_type[v_s..v_e];
                if (isValidBoundary(cand)) {
                    return cand;
                }
                return "";
            }
        }

        pos = seg_end;
        if (pos < content_type.len && content_type[pos] == ';' as u8) {
            pos += 1;
        }
    }
    return "";
}

fn matchBoundaryAt(payload: string, pos: usize, boundary: string) -> bool {
    const need: usize = boundary.len + 2;
    if (pos + need > payload.len) {
        return false;
    }
    if (payload[pos] != '-' as u8 || payload[pos + 1] != '-' as u8) {
        return false;
    }
    if (!strEq(payload[(pos + 2)..(pos + need)], boundary)) {
        return false;
    }
    const after: usize = pos + need;
    if (after == payload.len) {
        return true;
    }
    const c: u8 = payload[after];
    return c == '\r' as u8 || c == '\n' as u8 || c == ' ' as u8 || c == '\t' as u8
        || (c == '-' as u8 && after + 1 < payload.len && payload[after + 1] == '-' as u8);
}

fn extractDispositionParam(disp: string, key: string) -> string {
    let pos: usize = 0;
    while (pos < disp.len && disp[pos] != ';' as u8) {
        pos += 1;
    }
    if (pos >= disp.len) {
        return "";
    }
    pos += 1;

    while (pos < disp.len) {
        while (pos < disp.len && isWs(disp[pos])) {
            pos += 1;
        }
        let seg_end: usize = pos;
        let in_quote: bool = false;
        while (seg_end < disp.len) {
            const c: u8 = disp[seg_end];
            if (in_quote) {
                if (c == '\\' as u8 && seg_end + 1 < disp.len) {
                    seg_end += 2;
                    continue;
                }
                if (c == '"' as u8) {
                    in_quote = false;
                }
            } else {
                if (c == '"' as u8) {
                    in_quote = true;
                } else if (c == ';' as u8) {
                    break;
                }
            }
            seg_end += 1;
        }

        let eq: usize = pos;
        while (eq < seg_end && disp[eq] != '=' as u8) {
            eq += 1;
        }
        if (eq < seg_end) {
            let k_end: usize = eq;
            while (k_end > pos && isWs(disp[k_end - 1])) {
                k_end -= 1;
            }
            if (eqIgnoreCase(disp[pos..k_end], key)) {
                let v_s: usize = eq + 1;
                let v_e: usize = seg_end;
                while (v_s < v_e && isWs(disp[v_s])) {
                    v_s += 1;
                }
                while (v_e > v_s && isWs(disp[v_e - 1])) {
                    v_e -= 1;
                }
                if (v_e - v_s >= 2 && disp[v_s] == '"' as u8 && disp[v_e - 1] == '"' as u8) {
                    v_s += 1;
                    v_e -= 1;
                }
                return disp[v_s..v_e];
            }
        }

        pos = seg_end;
        if (pos < disp.len && disp[pos] == ';' as u8) {
            pos += 1;
        }
    }
    return "";
}

pub fn nextPart(payload: string, boundary: string, start: usize) -> MultipartPart {
    const fail = MultipartPart {
        headers: "",
        body: "",
        name: "",
        filename: "",
        content_type: "",
        next_pos: start,
        valid: false,
    };

    if (!isValidBoundary(boundary) || start >= payload.len) {
        return fail;
    }

    // Locate starting "--<boundary>"
    let d_pos: usize = start;
    let found_open: bool = false;
    while (d_pos + 2 + boundary.len <= payload.len) {
        if (matchBoundaryAt(payload, d_pos, boundary)) {
            if (d_pos == start || (d_pos >= 2 && payload[d_pos - 2] == '\r' as u8 && payload[d_pos - 1] == '\n' as u8)
                || (d_pos >= 1 && payload[d_pos - 1] == '\n' as u8)) {
                found_open = true;
                break;
            }
        }
        d_pos += 1;
    }
    if (!found_open) {
        return fail;
    }

    let pos: usize = d_pos + 2 + boundary.len;
    // Closing delimiter "--<boundary>--"
    if (pos + 2 <= payload.len && payload[pos] == '-' as u8 && payload[pos + 1] == '-' as u8) {
        return fail;
    }

    // Skip optional transport padding whitespace before CRLF
    while (pos < payload.len && isWs(payload[pos])) {
        pos += 1;
    }
    if (pos + 1 < payload.len && payload[pos] == '\r' as u8 && payload[pos + 1] == '\n' as u8) {
        pos += 2;
    } else if (pos < payload.len && payload[pos] == '\n' as u8) {
        pos += 1;
    } else {
        return fail;
    }

    // Split part headers and body
    const hdr_start: usize = pos;
    let hdr_end: usize = hdr_start;
    let body_start: usize = hdr_start;

    if (pos + 1 < payload.len && payload[pos] == '\r' as u8 && payload[pos + 1] == '\n' as u8) {
        hdr_end = pos;
        body_start = pos + 2;
    } else {
        let sep_found: bool = false;
        let scan: usize = pos;
        while (scan + 3 < payload.len) {
            if (payload[scan] == '\r' as u8 && payload[scan + 1] == '\n' as u8
                && payload[scan + 2] == '\r' as u8 && payload[scan + 3] == '\n' as u8) {
                hdr_end = scan;
                body_start = scan + 4;
                sep_found = true;
                break;
            }
            scan += 1;
        }
        if (!sep_found) {
            return fail;
        }
    }

    // Locate next "\r\n--<boundary>" marking end of body
    let b_scan: usize = body_start;
    let found_close: bool = false;
    let body_end: usize = body_start;
    let next_p: usize = payload.len;

    while (b_scan + 4 + boundary.len <= payload.len) {
        if (payload[b_scan] == '\r' as u8 && payload[b_scan + 1] == '\n' as u8
            && matchBoundaryAt(payload, b_scan + 2, boundary)) {
            body_end = b_scan;
            next_p = b_scan + 2;
            found_close = true;
            break;
        }
        b_scan += 1;
    }
    if (!found_close) {
        return fail;
    }

    // Parse part headers for Content-Disposition (name, filename) and Content-Type
    const headers_slice: string = payload[hdr_start..hdr_end];
    let p_name: string = "";
    let p_filename: string = "";
    let p_ctype: string = "";

    let hp: usize = 0;
    while (hp < headers_slice.len) {
        let le: usize = hp;
        while (le < headers_slice.len && headers_slice[le] != '\n' as u8) {
            le += 1;
        }
        let re: usize = le;
        if (re > hp && headers_slice[re - 1] == '\r' as u8) {
            re -= 1;
        }

        let colon: usize = hp;
        while (colon < re && headers_slice[colon] != ':' as u8) {
            colon += 1;
        }
        if (colon < re) {
            const h_key: string = headers_slice[hp..colon];
            let vs: usize = colon + 1;
            let ve: usize = re;
            while (vs < ve && isWs(headers_slice[vs])) {
                vs += 1;
            }
            while (ve > vs && isWs(headers_slice[ve - 1])) {
                ve -= 1;
            }
            const h_val: string = headers_slice[vs..ve];
            if (eqIgnoreCase(h_key, "content-disposition")) {
                p_name = extractDispositionParam(h_val, "name");
                p_filename = extractDispositionParam(h_val, "filename");
            } else if (eqIgnoreCase(h_key, "content-type")) {
                p_ctype = h_val;
            }
        }

        hp = le;
        if (hp < headers_slice.len && headers_slice[hp] == '\n' as u8) {
            hp += 1;
        }
    }

    return MultipartPart {
        headers: headers_slice,
        body: payload[body_start..body_end],
        name: p_name,
        filename: p_filename,
        content_type: p_ctype,
        next_pos: next_p,
        valid: true,
    };
}

pub fn countParts(payload: string, boundary: string) -> usize {
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < payload.len) {
        const part: MultipartPart = nextPart(payload, boundary, pos);
        if (!part.valid) {
            break;
        }
        count += 1;
        if (part.next_pos <= pos) {
            break;
        }
        pos = part.next_pos;
    }
    return count;
}
