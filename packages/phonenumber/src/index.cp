// @cap/tel-uri — Zero-heap RFC 3966 tel: URI & E.164 phone number codec
// MIT License (clean-room CP implementation)

import { isDigit, eqIgnoreCase, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub struct TelUri {
    pub number: string,
    pub is_global: bool,
    pub ext: string,
    pub phone_context: string,
    pub valid: bool,
}

fn isVisualSep(c: u8) -> bool {
    return c == '-' as u8 || c == '.' as u8 || c == '(' as u8 || c == ')' as u8;
}

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8;
}

pub fn isValidE164(number: string) -> bool {
    // '+' followed by 7..15 digits, first digit non-zero
    if (number.len < 8 || number.len > 16) {
        return false;
    }
    if (number[0] != '+' as u8) {
        return false;
    }
    if (number[1] < '1' as u8 || number[1] > '9' as u8) {
        return false;
    }
    let i: usize = 2;
    while (i < number.len) {
        if (!isDigit(number[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn normalizeE164(input: string, out: []u8) -> usize {
    let i: usize = 0;
    while (i < input.len && isWs(input[i])) {
        i += 1;
    }
    if (i >= input.len || input[i] != '+' as u8) {
        return 0;
    }
    if (out.len < 1) {
        return 0;
    }
    out[0] = '+' as u8;
    let pos: usize = 1;
    i += 1;

    while (i < input.len) {
        const c: u8 = input[i];
        if (isVisualSep(c) || isWs(c)) {
            i += 1;
            continue;
        }
        if (!isDigit(c)) {
            return 0;
        }
        if (pos >= out.len || pos >= 16) {
            return 0;
        }
        out[pos] = c;
        pos += 1;
        i += 1;
    }

    if (!isValidE164(out[0..pos])) {
        return 0;
    }
    return pos;
}

pub fn parseTelUri(uri: string) -> TelUri {
    const fail = TelUri {
        number: "",
        is_global: false,
        ext: "",
        phone_context: "",
        valid: false,
    };

    if (uri.len < 5 || !eqIgnoreCase(uri[0..4], "tel:")) {
        return fail;
    }

    // Split number from optional ";param=val..."
    const semi: usize = findByte(uri, ';' as u8, 4) ?? uri.len;
    const num_part: string = uri[4..semi];
    if (num_part.len == 0) {
        return fail;
    }

    let is_global: bool = false;
    let scan_start: usize = 0;
    if (num_part[0] == '+' as u8) {
        is_global = true;
        scan_start = 1;
    }

    let digit_count: usize = 0;
    let i: usize = scan_start;
    while (i < num_part.len) {
        const c: u8 = num_part[i];
        if (isDigit(c)) {
            digit_count += 1;
        } else if (!isVisualSep(c)) {
            return fail;
        }
        i += 1;
    }
    if (digit_count == 0) {
        return fail;
    }

    let ext: string = "";
    let phone_ctx: string = "";

    let pos: usize = semi;
    while (pos < uri.len) {
        pos += 1; // skip ';'
        let seg_end: usize = pos;
        while (seg_end < uri.len && uri[seg_end] != ';' as u8) {
            seg_end += 1;
        }
        let eq: usize = pos;
        while (eq < seg_end && uri[eq] != '=' as u8) {
            eq += 1;
        }
        if (eq <= pos || eq + 1 >= seg_end) {
            return fail;
        }
        const k: string = uri[pos..eq];
        const v: string = uri[(eq + 1)..seg_end];
        if (eqIgnoreCase(k, "ext")) {
            ext = v;
        } else if (eqIgnoreCase(k, "phone-context")) {
            phone_ctx = v;
        }
        pos = seg_end;
    }

    // RFC 3966: local numbers MUST include phone-context
    if (!is_global && phone_ctx.len == 0) {
        return fail;
    }

    return TelUri {
        number: num_part,
        is_global: is_global,
        ext: ext,
        phone_context: phone_ctx,
        valid: true,
    };
}

fn appendSlice(out: []u8, pos: usize, s: string) -> i64 {
    if (s.len > out.len - pos) {
        return -1;
    }
    return writeStr(s, out, pos) as i64;
}

pub fn formatTelUri(e164_number: string, ext: string, out: []u8) -> usize {
    if (!isValidE164(e164_number)) {
        return 0;
    }
    let p: i64 = appendSlice(out, 0, "tel:");
    if (p < 0) {
        return 0;
    }
    p = appendSlice(out, p as usize, e164_number);
    if (p < 0) {
        return 0;
    }
    if (ext.len > 0) {
        p = appendSlice(out, p as usize, ";ext=");
        if (p < 0) {
            return 0;
        }
        p = appendSlice(out, p as usize, ext);
        if (p < 0) {
            return 0;
        }
    }
    return p as usize;
}
