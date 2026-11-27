// @cap/calver — Zero-allocation Calendar Versioning (CalVer) engine for CP
// Clean-room MIT implementation inspired by npm `calver` and Rust `calver`.

import { isDigit, isAlpha, toLowerByte, eqIgnoreCase, trim, findByte } from "cap:ascii";
import { writeStr, writeU64, parseU64 } from "cap:fmt";

pub enum CalverFormat: u8 {
    YyyyMmMicro = 0,
    YyMmMicro = 1,
    Yyyy0mMicro = 2,
    YyyyMmDd = 3,
    YyyyMmDdMicro = 4,
}

pub struct Calver {
    pub valid: bool,
    pub year: u16,
    pub month: u8,
    pub day: u8,
    pub micro: u32,
    pub modifier: string,
    pub modifier_num: u32,

    pub static fn invalid() -> Calver {
        return Calver{
            valid: false,
            year: 0,
            month: 0,
            day: 0,
            micro: 0,
            modifier: "",
            modifier_num: 0,
        };
    }
}

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

fn cmpIgnoreCase(a: string, b: string) -> i32 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        const ca = toLowerByte(a[i]);
        const cb = toLowerByte(b[i]);
        if (ca < cb) return -1;
        if (ca > cb) return 1;
        i += 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

fn isLeapYear(year: u16) -> bool {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

fn daysInMonth(year: u16, month: u8) -> u8 {
    switch (month) {
        1, 3, 5, 7, 8, 10, 12 => return 31,
        4, 6, 9, 11 => return 30,
        2 => return if (isLeapYear(year)) 29 else 28,
        else => return 0,
    }
}

fn parseU32Strict(s: string) -> u32? {
    if (s.len == 0 || s.len > 9 || s[0] == '+') {
        return null;
    }
    const v = parseU64(s);
    if (v == null) {
        return null;
    }
    return v.? as u32;
}

struct ModifierParts {
    ok: bool,
    name: string,
    num: u32,
}

fn parseModifierTail(s: string) -> ModifierParts {
    const bad = ModifierParts{ ok: false, name: "", num: 0 };
    if (s.len == 0 || !isAlpha(s[0])) {
        return bad;
    }
    let i: usize = 0;
    while (i < s.len && isAlpha(s[i])) {
        i += 1;
    }
    const name = s[0..i];
    if (i == s.len) {
        return ModifierParts{ ok: true, name: name, num: 0 };
    }
    let num_start = i;
    if (s[num_start] == '.' || s[num_start] == '-') {
        num_start += 1;
    }
    const num_opt = parseU32Strict(s[num_start..s.len]);
    if (num_opt == null) {
        return bad;
    }
    return ModifierParts{ ok: true, name: name, num: num_opt.? };
}

fn parseCalverCore(raw: string, force_day_on_3: bool) -> Calver {
    let s: string = trim(raw);
    if (s.len == 0) {
        return Calver.invalid();
    }
    if (s[0] == 'v' || s[0] == 'V') {
        s = s[1..s.len];
    }
    if (s.len == 0) {
        return Calver.invalid();
    }

    // Split optional modifier: either after '-' (`2026.11.0-alpha.1`) or inline alpha (`2026.11.0b2`)
    let core = s;
    let mod_name: string = "";
    let mod_num: u32 = 0;

    const dash_pos = findByte(s, '-', 0) ?? s.len;

    if (dash_pos < s.len) {
        core = s[0..dash_pos];
        const mp = parseModifierTail(s[(dash_pos + 1)..s.len]);
        if (!mp.ok) {
            return Calver.invalid();
        }
        mod_name = mp.name;
        mod_num = mp.num;
    } else {
        // Check for inline PEP 440 / CalVer suffix like `2026.11.0b2` or `2026.11.0.rc1`
        let alpha_pos: usize = s.len;
        let a_i: usize = 0;
        while (a_i < s.len) {
            if (isAlpha(s[a_i])) {
                alpha_pos = a_i;
                break;
            }
            a_i += 1;
        }
        if (alpha_pos < s.len) {
            let core_end = alpha_pos;
            if (core_end > 0 && s[core_end - 1] == '.') {
                core_end -= 1;
            }
            core = s[0..core_end];
            const mp = parseModifierTail(s[alpha_pos..s.len]);
            if (!mp.ok) {
                return Calver.invalid();
            }
            mod_name = mp.name;
            mod_num = mp.num;
        }
    }

    // Split `core` by '.' into 2, 3, or 4 numeric segments
    let segs: [4]string = ["", "", "", ""];
    let seg_count: usize = 0;
    let seg_start: usize = 0;
    let i: usize = 0;
    while (i <= core.len) {
        if (i == core.len || core[i] == '.') {
            if (seg_count >= 4) {
                return Calver.invalid();
            }
            segs[seg_count] = core[seg_start..i];
            seg_count += 1;
            seg_start = i + 1;
        }
        i += 1;
    }

    if (seg_count < 2) {
        return Calver.invalid();
    }

    // Year: 2-digit (2000..2099) or 4-digit (1970..9999)
    const year_str = segs[0];
    if (year_str.len != 2 && year_str.len != 4) {
        return Calver.invalid();
    }
    const year_raw = parseU32Strict(year_str);
    if (year_raw == null) {
        return Calver.invalid();
    }
    let year: u16 = 0;
    if (year_str.len == 2) {
        year = (2000 + year_raw.?) as u16;
    } else {
        if (year_raw.? < 1970 || year_raw.? > 9999) {
            return Calver.invalid();
        }
        year = year_raw.? as u16;
    }

    // Month: 1..12
    const month_str = segs[1];
    if (month_str.len == 0 || month_str.len > 2) {
        return Calver.invalid();
    }
    const month_raw = parseU32Strict(month_str);
    if (month_raw == null || month_raw.? < 1 || month_raw.? > 12) {
        return Calver.invalid();
    }
    const month = month_raw.? as u8;

    let day: u8 = 0;
    let micro: u32 = 0;

    if (seg_count == 3) {
        const s2 = parseU32Strict(segs[2]);
        if (s2 == null) {
            return Calver.invalid();
        }
        if (force_day_on_3) {
            if (s2.? < 1 || s2.? > (daysInMonth(year, month) as u32)) {
                return Calver.invalid();
            }
            day = s2.? as u8;
        } else {
            micro = s2.?;
        }
    } else if (seg_count == 4) {
        const s2 = parseU32Strict(segs[2]);
        const s3 = parseU32Strict(segs[3]);
        if (s2 == null || s3 == null) {
            return Calver.invalid();
        }
        if (s2.? < 1 || s2.? > (daysInMonth(year, month) as u32)) {
            return Calver.invalid();
        }
        day = s2.? as u8;
        micro = s3.?;
    }

    return Calver{
        valid: true,
        year: year,
        month: month,
        day: day,
        micro: micro,
        modifier: mod_name,
        modifier_num: mod_num,
    };
}

// ---------------------------------------------------------------------------
// Public API: Parse, Compare, Bump, Format
// ---------------------------------------------------------------------------

/// Parses a CalVer string (2/4-digit year, month, optional day/micro, optional pre-release modifier).
/// 4-segment versions (`YYYY.MM.DD.MICRO`) validate `day` against month/leap-year bounds.
pub fn parseCalver(raw: string) -> Calver {
    return parseCalverCore(raw, false);
}

/// Parses a CalVer string using an explicit `CalverFormat` (distinguishes `YYYY.MM.DD` from `YYYY.MM.MICRO`).
pub fn parseCalverWith(raw: string, fmt: CalverFormat) -> Calver {
    const is_day_3 = (fmt == CalverFormat.YyyyMmDd);
    return parseCalverCore(raw, is_day_3);
}

fn modifierRank(name: string) -> u8 {
    if (eqIgnoreCase(name, "dev")) return 1;
    if (eqIgnoreCase(name, "a") || eqIgnoreCase(name, "alpha")) return 2;
    if (eqIgnoreCase(name, "b") || eqIgnoreCase(name, "beta")) return 3;
    if (eqIgnoreCase(name, "rc")) return 4;
    return 5;
}

/// Compares two `Calver` values, returning `-1`, `0`, or `1`.
/// Pre-release versions (with `modifier`) always sort lower than the corresponding final release.
pub fn compareCalver(a: Calver, b: Calver) -> i32 {
    if (a.year != b.year) return if (a.year < b.year) -1 else 1;
    if (a.month != b.month) return if (a.month < b.month) -1 else 1;
    if (a.day != b.day) return if (a.day < b.day) -1 else 1;
    if (a.micro != b.micro) return if (a.micro < b.micro) -1 else 1;

    const a_pre = a.modifier.len > 0;
    const b_pre = b.modifier.len > 0;
    if (!a_pre && !b_pre) return 0;
    if (a_pre && !b_pre) return -1;
    if (!a_pre && b_pre) return 1;

    const ra = modifierRank(a.modifier);
    const rb = modifierRank(b.modifier);
    if (ra != rb) return if (ra < rb) -1 else 1;

    if (ra == 5) {
        const lex = cmpIgnoreCase(a.modifier, b.modifier);
        if (lex != 0) return lex;
    }

    if (a.modifier_num != b.modifier_num) {
        return if (a.modifier_num < b.modifier_num) -1 else 1;
    }
    return 0;
}

/// Computes the next CalVer release for calendar date `(year, month, day)`:
/// - If `cur` matches the same calendar period, increments `micro` by 1.
/// - If entering a new calendar period, resets `micro` to 0.
pub fn bumpCalver(cur: Calver, year: u16, month: u8, day: u8, use_day: bool) -> Calver {
    if (year < 1970 || year > 9999 || month < 1 || month > 12) {
        return Calver.invalid();
    }
    let target_day: u8 = 0;
    if (use_day) {
        if (day < 1 || day > daysInMonth(year, month)) {
            return Calver.invalid();
        }
        target_day = day;
    }

    let next_micro: u32 = 0;
    if (cur.valid && cur.year == year && cur.month == month && cur.day == target_day) {
        next_micro = cur.micro + 1;
    }

    return Calver{
        valid: true,
        year: year,
        month: month,
        day: target_day,
        micro: next_micro,
        modifier: "",
        modifier_num: 0,
    };
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn append2Digits(out: []u8, pos: usize, val: u32) -> usize {
    let p = appendByte(out, pos, ('0' as u32 + ((val / 10) % 10)) as u8);
    p = appendByte(out, p, ('0' as u32 + (val % 10)) as u8);
    return p;
}

/// Formats `v` into `out` according to `fmt` (zero heap allocation).
pub fn formatCalver(v: Calver, fmt: CalverFormat, out: []u8) -> string {
    if (!v.valid) {
        return out[0..0];
    }
    let pos: usize = 0;
    switch (fmt) {
        .YyyyMmMicro => {
            pos = writeU64(v.year as u64, out, pos);
            pos = appendByte(out, pos, '.');
            pos = writeU64(v.month as u64, out, pos);
            pos = appendByte(out, pos, '.');
            pos = writeU64(v.micro as u64, out, pos);
        },
        .YyMmMicro => {
            pos = append2Digits(out, pos, (v.year % 100) as u32);
            pos = appendByte(out, pos, '.');
            pos = writeU64(v.month as u64, out, pos);
            pos = appendByte(out, pos, '.');
            pos = writeU64(v.micro as u64, out, pos);
        },
        .Yyyy0mMicro => {
            pos = writeU64(v.year as u64, out, pos);
            pos = appendByte(out, pos, '.');
            pos = append2Digits(out, pos, v.month as u32);
            pos = appendByte(out, pos, '.');
            pos = writeU64(v.micro as u64, out, pos);
        },
        .YyyyMmDd => {
            pos = writeU64(v.year as u64, out, pos);
            pos = appendByte(out, pos, '.');
            pos = append2Digits(out, pos, v.month as u32);
            pos = appendByte(out, pos, '.');
            pos = append2Digits(out, pos, v.day as u32);
        },
        .YyyyMmDdMicro => {
            pos = writeU64(v.year as u64, out, pos);
            pos = appendByte(out, pos, '.');
            pos = append2Digits(out, pos, v.month as u32);
            pos = appendByte(out, pos, '.');
            pos = append2Digits(out, pos, v.day as u32);
            pos = appendByte(out, pos, '.');
            pos = writeU64(v.micro as u64, out, pos);
        },
    }

    if (v.modifier.len > 0) {
        pos = appendByte(out, pos, '-');
        pos = writeStr(v.modifier, out, pos);
        if (v.modifier_num > 0) {
            pos = appendByte(out, pos, '.');
            pos = writeU64(v.modifier_num as u64, out, pos);
        }
    }

    return out[0..pos];
}
