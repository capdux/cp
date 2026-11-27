// packages/@cap/vcard/src/index.cp
// Zero-allocation RFC 6350 vCard 4.0 (.vcf) formatter and parser.
// MIT clean-room pure CP implementation.

import { eqIgnoreCase } from "cap:ascii";

fn isSpaceOrTab(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8);
}

/// Input contact fields for generating an RFC 6350 vCard 4.0 payload.
pub struct VCardData {
    pub full_name: []const u8,
    pub org: []const u8,
    pub title: []const u8,
    pub email: []const u8,
    pub tel: []const u8,
    pub url: []const u8,
}

/// Zero-copy parsed vCard property byte spans into the original `.vcf` buffer.
pub struct ParsedVCard {
    pub fn_start: usize,
    pub fn_len: usize,
    pub org_start: usize,
    pub org_len: usize,
    pub title_start: usize,
    pub title_len: usize,
    pub email_start: usize,
    pub email_len: usize,
    pub tel_start: usize,
    pub tel_len: usize,
    pub url_start: usize,
    pub url_len: usize,
    pub version_start: usize,
    pub version_len: usize,
    pub valid: bool,
}

fn writeRaw(dst: []u8, pos: *usize, src: []const u8): bool {
    if (pos.* + src.len > dst.len) {
        return false;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos.*] = src[i];
        pos.* += 1;
        i += 1;
    }
    return true;
}

fn writeEscaped(dst: []u8, pos: *usize, src: []const u8): bool {
    let i: usize = 0;
    while (i < src.len) {
        const c = src[i];
        if (c == ('\r' as u8)) {
            i += 1;
            continue;
        }
        if (c == ('\\' as u8) || c == (',' as u8) || c == (';' as u8)) {
            if (pos.* + 2 > dst.len) {
                return false;
            }
            dst[pos.*] = '\\' as u8;
            dst[pos.* + 1] = c;
            pos.* += 2;
        } else if (c == ('\n' as u8)) {
            if (pos.* + 2 > dst.len) {
                return false;
            }
            dst[pos.*] = '\\' as u8;
            dst[pos.* + 1] = 'n' as u8;
            pos.* += 2;
        } else {
            if (pos.* + 1 > dst.len) {
                return false;
            }
            dst[pos.*] = c;
            pos.* += 1;
        }
        i += 1;
    }
    return true;
}

fn writeProp(dst: []u8, pos: *usize, propPrefix: []const u8, val: []const u8): bool {
    if (!writeRaw(dst, pos, propPrefix)) {
        return false;
    }
    if (!writeEscaped(dst, pos, val)) {
        return false;
    }
    if (!writeRaw(dst, pos, "\r\n")) {
        return false;
    }
    return true;
}

/// Format a `VCardData` into RFC 6350 vCard 4.0 format with CRLF line endings.
/// Escapes `\`, `,`, `;`, and `\n`. Omits empty optional fields.
/// Returns bytes written on success, or `-1` if `full_name` is empty or `dst` is too small.
pub fn formatVCard(card: VCardData, dst: []u8): i64 {
    if (card.full_name.len == 0) {
        return -1 as i64;
    }

    let pos: usize = 0;
    if (!writeRaw(dst, &pos, "BEGIN:VCARD\r\nVERSION:4.0\r\n")) {
        return -1 as i64;
    }
    if (!writeProp(dst, &pos, "FN:", card.full_name)) {
        return -1 as i64;
    }
    if (card.org.len > 0) {
        if (!writeProp(dst, &pos, "ORG:", card.org)) {
            return -1 as i64;
        }
    }
    if (card.title.len > 0) {
        if (!writeProp(dst, &pos, "TITLE:", card.title)) {
            return -1 as i64;
        }
    }
    if (card.email.len > 0) {
        if (!writeProp(dst, &pos, "EMAIL:", card.email)) {
            return -1 as i64;
        }
    }
    if (card.tel.len > 0) {
        if (!writeProp(dst, &pos, "TEL:", card.tel)) {
            return -1 as i64;
        }
    }
    if (card.url.len > 0) {
        if (!writeProp(dst, &pos, "URL:", card.url)) {
            return -1 as i64;
        }
    }
    if (!writeRaw(dst, &pos, "END:VCARD\r\n")) {
        return -1 as i64;
    }

    return pos as i64;
}

/// Zero-copy parser for the first `BEGIN:VCARD`..`END:VCARD` block in `vcf`.
/// Supports parameterized properties such as `TEL;TYPE=work,voice:+1-555-0100`
/// and `EMAIL;TYPE=internet:a@b.com`.
pub fn parseVCard(vcf: []const u8): ParsedVCard {
    let res = ParsedVCard{
        fn_start: 0,
        fn_len: 0,
        org_start: 0,
        org_len: 0,
        title_start: 0,
        title_len: 0,
        email_start: 0,
        email_len: 0,
        tel_start: 0,
        tel_len: 0,
        url_start: 0,
        url_len: 0,
        version_start: 0,
        version_len: 0,
        valid: false,
    };

    let inCard = false;
    let foundEnd = false;
    let pos: usize = 0;

    while (pos < vcf.len) {
        let lineEnd = pos;
        while (lineEnd < vcf.len && vcf[lineEnd] != ('\n' as u8)) {
            lineEnd += 1;
        }
        let cleanEnd = lineEnd;
        if (cleanEnd > pos && vcf[cleanEnd - 1] == ('\r' as u8)) {
            cleanEnd -= 1;
        }

        const lineStart = pos;
        if (lineEnd < vcf.len) {
            pos = lineEnd + 1;
        } else {
            pos = vcf.len;
        }

        // Trim leading spaces/tabs on property name side.
        let s = lineStart;
        while (s < cleanEnd && isSpaceOrTab(vcf[s])) {
            s += 1;
        }
        if (s >= cleanEnd) {
            continue;
        }

        // Find first ':' separating property+params from value.
        let colon = s;
        while (colon < cleanEnd && vcf[colon] != (':' as u8)) {
            colon += 1;
        }
        if (colon >= cleanEnd) {
            continue;
        }

        // Extract property name before first ';' (if parameterized).
        let nameEnd = s;
        while (nameEnd < colon && vcf[nameEnd] != (';' as u8)) {
            nameEnd += 1;
        }
        // Strip optional group prefix (e.g. "item1.EMAIL" -> "EMAIL").
        let nameStart = s;
        let di = s;
        while (di < nameEnd) {
            if (vcf[di] == ('.' as u8)) {
                nameStart = di + 1;
            }
            di += 1;
        }
        while (nameEnd > nameStart && isSpaceOrTab(vcf[nameEnd - 1])) {
            nameEnd -= 1;
        }
        const propName = vcf[nameStart..nameEnd];

        const valStart = colon + 1;
        const valLen = cleanEnd - valStart;
        const valSlice = vcf[valStart..cleanEnd];

        if (!inCard) {
            if (eqIgnoreCase(propName, "BEGIN")) {
                // Trim trailing spaces on BEGIN value.
                let ve = cleanEnd;
                while (ve > valStart && isSpaceOrTab(vcf[ve - 1])) {
                    ve -= 1;
                }
                let vs = valStart;
                while (vs < ve && isSpaceOrTab(vcf[vs])) {
                    vs += 1;
                }
                if (eqIgnoreCase(vcf[vs..ve], "VCARD")) {
                    inCard = true;
                }
            }
            continue;
        }

        if (eqIgnoreCase(propName, "END")) {
            let ve = cleanEnd;
            while (ve > valStart && isSpaceOrTab(vcf[ve - 1])) {
                ve -= 1;
            }
            let vs = valStart;
            while (vs < ve && isSpaceOrTab(vcf[vs])) {
                vs += 1;
            }
            if (eqIgnoreCase(vcf[vs..ve], "VCARD")) {
                foundEnd = true;
                break;
            }
        } else if (eqIgnoreCase(propName, "VERSION")) {
            if (res.version_len == 0 && valSlice.len > 0) {
                res.version_start = valStart;
                res.version_len = valLen;
            }
        } else if (eqIgnoreCase(propName, "FN")) {
            if (res.fn_len == 0 && valSlice.len > 0) {
                res.fn_start = valStart;
                res.fn_len = valLen;
            }
        } else if (eqIgnoreCase(propName, "ORG")) {
            if (res.org_len == 0 && valSlice.len > 0) {
                res.org_start = valStart;
                res.org_len = valLen;
            }
        } else if (eqIgnoreCase(propName, "TITLE")) {
            if (res.title_len == 0 && valSlice.len > 0) {
                res.title_start = valStart;
                res.title_len = valLen;
            }
        } else if (eqIgnoreCase(propName, "EMAIL")) {
            if (res.email_len == 0 && valSlice.len > 0) {
                res.email_start = valStart;
                res.email_len = valLen;
            }
        } else if (eqIgnoreCase(propName, "TEL")) {
            if (res.tel_len == 0 && valSlice.len > 0) {
                res.tel_start = valStart;
                res.tel_len = valLen;
            }
        } else if (eqIgnoreCase(propName, "URL")) {
            if (res.url_len == 0 && valSlice.len > 0) {
                res.url_start = valStart;
                res.url_len = valLen;
            }
        }
    }

    if (inCard && foundEnd) {
        res.valid = true;
    }
    return res;
}

export {
    VCardData,
    ParsedVCard,
    formatVCard,
    parseVCard,
};
