// @cap/urn-parse — Zero-allocation RFC 8141 URN Parser, Equivalence & Formatter
// Clean-room MIT implementation in pure CP.

import { isAlnum, toLowerByte, eqIgnoreCase, strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub struct Urn {
    pub nid: string,
    pub nss: string,
    pub r_component: string,
    pub q_component: string,
    pub f_component: string,
    pub valid: bool,
}

fn invalidUrn() -> Urn {
    return Urn {
        nid: "",
        nss: "",
        r_component: "",
        q_component: "",
        f_component: "",
        valid: false,
    };
}

fn urnIsSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

/// Validates an RFC 8141 Namespace Identifier (NID):
/// length 2..32, first & last characters ASCII alphanumeric, middle characters alphanumeric or `-`.
pub fn isValidNid(nid: string) -> bool {
    if (nid.len < 2 || nid.len > 32) {
        return false;
    }
    if (!isAlnum(nid[0]) || !isAlnum(nid[nid.len - 1])) {
        return false;
    }
    let i: usize = 1;
    while (i + 1 < nid.len) {
        const ch = nid[i];
        if (!isAlnum(ch) && ch != 45) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Parses an RFC 8141 URN (`urn:<nid>:<nss>[?+r][?=q][#f]`) with zero heap allocation.
pub fn parseUrn(input: string) -> Urn {
    let a: usize = 0;
    let b: usize = input.len;
    while (a < b && urnIsSpace(input[a])) {
        a += 1;
    }
    while (b > a && urnIsSpace(input[b - 1])) {
        b -= 1;
    }
    const s = input[a..b];
    if (s.len < 7 || !eqIgnoreCase(s[0..4], "urn:")) {
        return invalidUrn();
    }

    // 1. Strip optional "#f-component"
    let f_comp: string = "";
    let main_end = s.len;
    let hi: usize = 4;
    while (hi < s.len && s[hi] != 35) {
        hi += 1; // '#'
    }
    if (hi < s.len) {
        f_comp = s[hi + 1..s.len];
        main_end = hi;
    }

    // 2. Split NID and rest at first ':' after "urn:"
    let colon: usize = 4;
    while (colon < main_end && s[colon] != 58) {
        colon += 1;
    }
    if (colon >= main_end) {
        return invalidUrn();
    }
    const nid = s[4..colon];
    if (!isValidNid(nid)) {
        return invalidUrn();
    }

    const rest = s[colon + 1..main_end];
    if (rest.len == 0) {
        return invalidUrn();
    }

    // 3. Split optional "?+r-component" and "?=q-component"
    let nss = rest;
    let r_comp: string = "";
    let q_comp: string = "";

    let i: usize = 0;
    let found_r = false;
    let found_q = false;
    let r_pos: usize = rest.len;
    let q_pos: usize = rest.len;

    while (i + 1 < rest.len) {
        if (rest[i] == 63) {
            // '?'
            if (rest[i + 1] == 43 && !found_r && !found_q) {
                // "?+"
                found_r = true;
                r_pos = i;
                i += 2;
                continue;
            }
            if (rest[i + 1] == 61 && !found_q) {
                // "?="
                found_q = true;
                q_pos = i;
                break;
            }
        }
        i += 1;
    }

    if (found_r && found_q) {
        nss = rest[0..r_pos];
        r_comp = rest[r_pos + 2..q_pos];
        q_comp = rest[q_pos + 2..rest.len];
        if (r_comp.len == 0 || q_comp.len == 0) {
            return invalidUrn();
        }
    } else if (found_r) {
        nss = rest[0..r_pos];
        r_comp = rest[r_pos + 2..rest.len];
        if (r_comp.len == 0) {
            return invalidUrn();
        }
    } else if (found_q) {
        nss = rest[0..q_pos];
        q_comp = rest[q_pos + 2..rest.len];
        if (q_comp.len == 0) {
            return invalidUrn();
        }
    }

    if (nss.len == 0) {
        return invalidUrn();
    }

    return Urn {
        nid: nid,
        nss: nss,
        r_component: r_comp,
        q_component: q_comp,
        f_component: f_comp,
        valid: true,
    };
}

/// RFC 8141 Section 3.1 URN-equivalence:
/// case-insensitive `nid` equality and byte-exact `nss` equality, ignoring r/q/f components.
pub fn urnEquivalent(a: Urn, b: Urn) -> bool {
    if (!a.valid || !b.valid) {
        return false;
    }
    return eqIgnoreCase(a.nid, b.nid) && strEq(a.nss, b.nss);
}

fn pushSlice(dst: []u8, pos: usize, src: string) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    return writeStr(src, dst, pos) as i64;
}

fn pushLowerSlice(dst: []u8, pos: usize, src: string) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = toLowerByte(src[i]);
        i += 1;
    }
    return (pos + src.len) as i64;
}

/// Serializes a `Urn` into canonical lowercase-scheme & lowercase-NID form:
/// `urn:<lower-nid>:<nss>[?+r][?=q][#f]`. Returns bytes written, or `0` if invalid or buffer too small.
pub fn formatUrn(u: Urn, out: []u8) -> usize {
    if (!isValidNid(u.nid) || u.nss.len == 0) {
        return 0;
    }
    let p = pushSlice(out, 0, "urn:");
    if (p < 0) { return 0; }
    p = pushLowerSlice(out, p as usize, u.nid);
    if (p < 0) { return 0; }
    p = pushSlice(out, p as usize, ":");
    if (p < 0) { return 0; }
    p = pushSlice(out, p as usize, u.nss);
    if (p < 0) { return 0; }

    if (u.r_component.len > 0) {
        p = pushSlice(out, p as usize, "?+");
        if (p < 0) { return 0; }
        p = pushSlice(out, p as usize, u.r_component);
        if (p < 0) { return 0; }
    }
    if (u.q_component.len > 0) {
        p = pushSlice(out, p as usize, "?=");
        if (p < 0) { return 0; }
        p = pushSlice(out, p as usize, u.q_component);
        if (p < 0) { return 0; }
    }
    if (u.f_component.len > 0) {
        p = pushSlice(out, p as usize, "#");
        if (p < 0) { return 0; }
        p = pushSlice(out, p as usize, u.f_component);
        if (p < 0) { return 0; }
    }

    return p as usize;
}
