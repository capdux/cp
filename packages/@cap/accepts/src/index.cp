// @cap/accepts — Zero-allocation RFC 9110 HTTP content negotiation engine for CP
// Clean-room MIT implementation inspired by npm `accepts` and `negotiator` (75M+/wk).

fn isSpace(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + (32 as u8);
    }
    return c;
}

fn trimSlice(s: []const u8) -> []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isSpace(s[end - (1 as usize)])) {
        end -= 1;
    }
    return s[start..end];
}

fn eqIgnoreCase(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isStar(s: []const u8) -> bool {
    return s.len == (1 as usize) && s[0] == '*';
}

// Parse RFC 9110 q-value into integer milli-units 0..1000 (-1 on invalid)
pub fn parseQValue(s_in: []const u8) -> i32 {
    const s = trimSlice(s_in);
    if (s.len == (0 as usize) || s.len > (5 as usize)) {
        return -1 as i32;
    }
    const first = s[0];
    if (first != '0' && first != '1') {
        return -1 as i32;
    }
    if (s.len == (1 as usize)) {
        if (first == '1') {
            return 1000 as i32;
        }
        return 0 as i32;
    }
    if (s[1] != '.') {
        return -1 as i32;
    }
    if (first == '1') {
        let i: usize = 2;
        while (i < s.len) {
            if (s[i] != '0') {
                return -1 as i32;
            }
            i += 1;
        }
        return 1000 as i32;
    }

    // first == '0', s[1] == '.'
    let val: u32 = 0;
    let weight: u32 = 100;
    let i: usize = 2;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return -1 as i32;
        }
        val += ((c - '0') as u32) * weight;
        weight = weight / (10 as u32);
        i += 1;
    }
    return val as i32;
}

struct ParsedItem {
    token: []const u8,
    q: i32,
    valid: bool,
}

fn parseHeaderItem(raw: []const u8) -> ParsedItem {
    const s = trimSlice(raw);
    if (s.len == (0 as usize)) {
        return ParsedItem{ token: s, q: 0 as i32, valid: false };
    }

    // Find first ';'
    let semi: usize = s.len;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ';') {
            semi = i;
            break;
        }
        i += 1;
    }
    const tok = trimSlice(s[0..semi]);
    if (tok.len == (0 as usize)) {
        return ParsedItem{ token: tok, q: 0 as i32, valid: false };
    }

    let q: i32 = 1000;
    let pos = semi;
    while (pos < s.len) {
        // Advance past ';'
        pos += 1;
        const pStart = pos;
        while (pos < s.len && s[pos] != ';') {
            pos += 1;
        }
        const param = trimSlice(s[pStart..pos]);
        // Check for "q=..."
        let eqIdx: usize = param.len;
        let k: usize = 0;
        while (k < param.len) {
            if (param[k] == '=') {
                eqIdx = k;
                break;
            }
            k += 1;
        }
        if (eqIdx < param.len) {
            const pName = trimSlice(param[0..eqIdx]);
            if (pName.len == (1 as usize) && (pName[0] == 'q' || pName[0] == 'Q')) {
                const pVal = trimSlice(param[(eqIdx + (1 as usize))..param.len]);
                const parsedQ = parseQValue(pVal);
                if (parsedQ < (0 as i32)) {
                    return ParsedItem{ token: tok, q: 0 as i32, valid: false };
                }
                q = parsedQ;
            }
        }
    }

    return ParsedItem{ token: tok, q: q, valid: true };
}

fn matchEncodingSpec(headerTok: []const u8, cand: []const u8) -> u8 {
    if (eqIgnoreCase(headerTok, cand)) {
        return 2 as u8;
    }
    if (isStar(headerTok)) {
        return 1 as u8;
    }
    return 0 as u8;
}

fn isLangPrefix(prefix: []const u8, full: []const u8) -> bool {
    if (prefix.len == (0 as usize) || prefix.len >= full.len) {
        return false;
    }
    if (full[prefix.len] != '-') {
        return false;
    }
    return eqIgnoreCase(prefix, full[0..prefix.len]);
}

fn matchLanguageSpec(headerTok: []const u8, cand: []const u8) -> u8 {
    if (eqIgnoreCase(headerTok, cand)) {
        return 3 as u8;
    }
    if (isLangPrefix(cand, headerTok) || isLangPrefix(headerTok, cand)) {
        return 2 as u8;
    }
    if (isStar(headerTok)) {
        return 1 as u8;
    }
    return 0 as u8;
}

fn findSlash(s: []const u8) -> usize {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '/') {
            return i;
        }
        i += 1;
    }
    return s.len;
}

fn matchMediaSpec(headerTok: []const u8, cand: []const u8) -> u8 {
    const hs = findSlash(headerTok);
    const cs = findSlash(cand);
    if (hs == headerTok.len || cs == cand.len) {
        return 0 as u8;
    }
    const hType = trimSlice(headerTok[0..hs]);
    const hSub = trimSlice(headerTok[(hs + (1 as usize))..headerTok.len]);
    const cType = trimSlice(cand[0..cs]);
    const cSub = trimSlice(cand[(cs + (1 as usize))..cand.len]);

    if (isStar(hType) && isStar(hSub)) {
        return 1 as u8;
    }
    if (eqIgnoreCase(hType, cType)) {
        if (eqIgnoreCase(hSub, cSub)) {
            return 3 as u8;
        }
        if (isStar(hSub)) {
            return 2 as u8;
        }
    }
    return 0 as u8;
}

const MODE_ENCODING: u8 = 0;
const MODE_LANGUAGE: u8 = 1;
const MODE_MEDIA: u8 = 2;

fn matchByMode(mode: u8, headerTok: []const u8, cand: []const u8) -> u8 {
    if (mode == MODE_ENCODING) {
        return matchEncodingSpec(headerTok, cand);
    }
    if (mode == MODE_LANGUAGE) {
        return matchLanguageSpec(headerTok, cand);
    }
    return matchMediaSpec(headerTok, cand);
}

fn negotiateCore(accept_header: []const u8, available: []const []const u8, mode: u8) -> i64 {
    if (available.len == (0 as usize)) {
        return -1 as i64;
    }
    const trimmedHeader = trimSlice(accept_header);
    if (trimmedHeader.len == (0 as usize)) {
        return 0 as i64;
    }

    let bestIdx: i64 = -1;
    let bestQ: i32 = 0;
    let bestSpec: u8 = 0;

    let cIdx: usize = 0;
    while (cIdx < available.len) {
        const cand = trimSlice(available[cIdx]);
        let candSpec: u8 = 0;
        let candQ: i32 = -1;

        let pos: usize = 0;
        while (pos <= trimmedHeader.len) {
            const start = pos;
            while (pos < trimmedHeader.len && trimmedHeader[pos] != ',') {
                pos += 1;
            }
            const item = parseHeaderItem(trimmedHeader[start..pos]);
            if (item.valid) {
                const spec = matchByMode(mode, item.token, cand);
                if (spec > candSpec || (spec == candSpec && spec > (0 as u8) && item.q > candQ)) {
                    candSpec = spec;
                    candQ = item.q;
                }
            }
            pos += 1;
        }

        // Only accept if matched and not explicitly rejected with q=0
        if (candSpec > (0 as u8) && candQ > (0 as i32)) {
            if (bestIdx < (0 as i64) || candQ > bestQ || (candQ == bestQ && candSpec > bestSpec)) {
                bestIdx = cIdx as i64;
                bestQ = candQ;
                bestSpec = candSpec;
            }
        }
        cIdx += 1;
    }

    return bestIdx;
}

pub fn negotiateEncoding(accept_header: []const u8, available: []const []const u8) -> i64 {
    return negotiateCore(accept_header, available, MODE_ENCODING);
}

pub fn negotiateLanguage(accept_header: []const u8, available: []const []const u8) -> i64 {
    return negotiateCore(accept_header, available, MODE_LANGUAGE);
}

pub fn negotiateMediaType(accept_header: []const u8, available: []const []const u8) -> i64 {
    return negotiateCore(accept_header, available, MODE_MEDIA);
}

export {
    parseQValue,
    negotiateEncoding,
    negotiateLanguage,
    negotiateMediaType,
};
