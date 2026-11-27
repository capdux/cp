// @cap/cron-desc — Zero-allocation Cron Expression Human-Readable Descriptor (EN / ZH)
// Clean-room pure-CP implementation modeled after npm `cronstrue` and Rust `cron-descriptor` (MIT License).
//
// Features:
// - Standard 5-field Cron validation & parsing: `minute hour dom month dow`
// - Predefined macros: `@yearly`, `@annually`, `@monthly`, `@weekly`, `@daily`, `@midnight`, `@hourly`
// - Supports `*`, fixed values, `*/step` & `a-b/step`, `a-b` ranges, `a,b,c` lists,
//   month names (`JAN`-`DEC`), weekday names (`SUN`-`SAT`) & `0-7` (`0` and `7` = Sunday)
// - Bilingual output (`Locale.En` / `Locale.Zh`) & 24h / 12h clock formatting
// - 100% zero heap allocations: writes directly into caller-provided `out: []u8`

/// Output natural language locale.
pub enum Locale: u8 {
    En = 0,
    Zh = 1,
}

/// Formatting options for `describeCronWith`.
pub struct CronDescOptions {
    locale: Locale = Locale.En,
    use_24h: bool = true,

    /// Default English 24-hour options.
    pub static fn english(): CronDescOptions {
        return CronDescOptions{
            locale: Locale.En,
            use_24h: true,
        };
    }

    /// Default Chinese 24-hour options.
    pub static fn chinese(): CronDescOptions {
        return CronDescOptions{
            locale: Locale.Zh,
            use_24h: true,
        };
    }
}

const FIELD_ANY: u8 = 0;
const FIELD_VALUE: u8 = 1;
const FIELD_STEP: u8 = 2;
const FIELD_RANGE: u8 = 3;
const FIELD_LIST: u8 = 4;

const MAX_LIST_ITEMS: usize = 16;

struct FieldItem {
    is_any: bool = false,
    is_range: bool = false,
    start: u8 = 0,
    end: u8 = 0,
    step: u8 = 0,
}

struct CronField {
    kind: u8 = FIELD_ANY,
    items: [16]FieldItem = [
        FieldItem{}, FieldItem{}, FieldItem{}, FieldItem{},
        FieldItem{}, FieldItem{}, FieldItem{}, FieldItem{},
        FieldItem{}, FieldItem{}, FieldItem{}, FieldItem{},
        FieldItem{}, FieldItem{}, FieldItem{}, FieldItem{},
    ],
    item_len: u8 = 0,
}

struct ParsedCron {
    valid: bool = false,
    minute: CronField = CronField{},
    hour: CronField = CronField{},
    dom: CronField = CronField{},
    month: CronField = CronField{},
    dow: CronField = CronField{},
}

const MONTHS_EN: [12]string = [
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December",
];

const MONTHS_ZH: [12]string = [
    "1 月", "2 月", "3 月", "4 月", "5 月", "6 月",
    "7 月", "8 月", "9 月", "10 月", "11 月", "12 月",
];

const MONTHS_ABBR: [12]string = [
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC",
];

const DAYS_EN: [7]string = [
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday",
];

const DAYS_ZH: [7]string = [
    "周日", "周一", "周二", "周三", "周四", "周五", "周六",
];

const DAYS_ABBR: [7]string = [
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT",
];

fn toUpperAscii(c: u8): u8 {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn eqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toUpperAscii(a[i]) != toUpperAscii(b[i])) return false;
    }
    return true;
}

fn pushByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pushStr(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn pushU8(out: []u8, pos: usize, val: u8): usize {
    let p = pos;
    if (val >= 100) {
        p = pushByte(out, p, (48 as u8) + (val / 100));
        p = pushByte(out, p, (48 as u8) + ((val / 10) % 10));
        p = pushByte(out, p, (48 as u8) + (val % 10));
        return p;
    }
    if (val >= 10) {
        p = pushByte(out, p, (48 as u8) + (val / 10));
        p = pushByte(out, p, (48 as u8) + (val % 10));
        return p;
    }
    return pushByte(out, p, (48 as u8) + val);
}

fn pushU8Pad2(out: []u8, pos: usize, val: u8): usize {
    let p = pos;
    p = pushByte(out, p, (48 as u8) + ((val / 10) % 10));
    p = pushByte(out, p, (48 as u8) + (val % 10));
    return p;
}

fn pushTime(out: []u8, pos: usize, hour: u8, minute: u8, opts: CronDescOptions): usize {
    let p = pos;
    if (opts.use_24h) {
        p = pushU8Pad2(out, p, hour);
        p = pushByte(out, p, ':');
        p = pushU8Pad2(out, p, minute);
        return p;
    }
    // 12-hour clock
    if (opts.locale == Locale.Zh) {
        if (hour == 0) {
            p = pushStr(out, p, "凌晨 12:");
        } else if (hour < 12) {
            p = pushStr(out, p, "上午 ");
            p = pushU8(out, p, hour);
            p = pushByte(out, p, ':');
        } else if (hour == 12) {
            p = pushStr(out, p, "中午 12:");
        } else {
            p = pushStr(out, p, "下午 ");
            p = pushU8(out, p, hour - 12);
            p = pushByte(out, p, ':');
        }
        p = pushU8Pad2(out, p, minute);
        return p;
    }
    // English 12h
    let h12: u8 = hour;
    let is_pm = false;
    if (hour == 0) {
        h12 = 12;
        is_pm = false;
    } else if (hour < 12) {
        h12 = hour;
        is_pm = false;
    } else if (hour == 12) {
        h12 = 12;
        is_pm = true;
    } else {
        h12 = hour - 12;
        is_pm = true;
    }
    p = pushU8(out, p, h12);
    p = pushByte(out, p, ':');
    p = pushU8Pad2(out, p, minute);
    if (is_pm) {
        p = pushStr(out, p, " PM");
    } else {
        p = pushStr(out, p, " AM");
    }
    return p;
}

fn dayName(val: u8, locale: Locale): string {
    const idx = (val % 7) as usize;
    if (locale == Locale.Zh) return DAYS_ZH[idx];
    return DAYS_EN[idx];
}

fn monthName(val: u8, locale: Locale): string {
    if (val < 1 || val > 12) return "";
    const idx = (val - 1) as usize;
    if (locale == Locale.Zh) return MONTHS_ZH[idx];
    return MONTHS_EN[idx];
}

/// Parse a single numeric or named atom (e.g. "15", "MON", "JAN"). Returns `-1` on failure.
fn parseAtom(tok: string, min_val: u8, max_val: u8, allow_month: bool, allow_dow: bool): i32 {
    if (tok.len == 0) return -1;

    if (allow_month && tok.len == 3) {
        let i: usize = 0;
        while (i < 12) : (i += 1) {
            if (eqIgnoreCase(tok, MONTHS_ABBR[i])) {
                return (i + 1) as i32;
            }
        }
    }
    if (allow_dow && tok.len == 3) {
        let i: usize = 0;
        while (i < 7) : (i += 1) {
            if (eqIgnoreCase(tok, DAYS_ABBR[i])) {
                return i as i32;
            }
        }
    }

    // Must be pure ASCII digits
    if (tok.len > 3) return -1;
    let val: i32 = 0;
    let i: usize = 0;
    while (i < tok.len) : (i += 1) {
        if (!isDigit(tok[i])) return -1;
        val = val * 10 + ((tok[i] - (48 as u8)) as i32);
    }
    if (val < (min_val as i32) || val > (max_val as i32)) return -1;
    return val;
}

struct ItemParseRes {
    ok: bool,
    item: FieldItem,
}

fn parseFieldItem(tok: string, min_val: u8, max_val: u8, allow_month: bool, allow_dow: bool): ItemParseRes {
    let fail = ItemParseRes{ ok: false, item: FieldItem{} };
    if (tok.len == 0) return fail;

    // Split optional `/step`
    let slash_idx: i32 = -1;
    let i: usize = 0;
    while (i < tok.len) : (i += 1) {
        if (tok[i] == '/') {
            if (slash_idx >= 0) return fail; // multiple slashes
            slash_idx = i as i32;
        }
    }

    let base_tok = tok;
    let step: u8 = 0;
    if (slash_idx >= 0) {
        const sidx = slash_idx as usize;
        base_tok = tok[0..sidx];
        const step_tok = tok[(sidx + 1)..tok.len];
        if (base_tok.len == 0 || step_tok.len == 0 || step_tok.len > 3) return fail;
        let sv: i32 = 0;
        let k: usize = 0;
        while (k < step_tok.len) : (k += 1) {
            if (!isDigit(step_tok[k])) return fail;
            sv = sv * 10 + ((step_tok[k] - (48 as u8)) as i32);
        }
        if (sv <= 0 || sv > (max_val as i32)) return fail;
        step = sv as u8;
    }

    // Wildcard `*`
    if (base_tok.len == 1 && base_tok[0] == '*') {
        return ItemParseRes{
            ok: true,
            item: FieldItem{
                is_any: true,
                is_range: false,
                start: min_val,
                end: max_val,
                step: step,
            },
        };
    }

    // Check for range `a-b`
    let dash_idx: i32 = -1;
    let j: usize = 0;
    while (j < base_tok.len) : (j += 1) {
        if (base_tok[j] == '-') {
            if (dash_idx >= 0) return fail; // multiple dashes
            dash_idx = j as i32;
        }
    }

    if (dash_idx >= 0) {
        const didx = dash_idx as usize;
        const left = base_tok[0..didx];
        const right = base_tok[(didx + 1)..base_tok.len];
        const s = parseAtom(left, min_val, max_val, allow_month, allow_dow);
        const e = parseAtom(right, min_val, max_val, allow_month, allow_dow);
        if (s < 0 || e < 0) return fail;
        // For DOW, allow 5-7 (Fri-Sun); otherwise require s <= e
        if (s > e) return fail;
        return ItemParseRes{
            ok: true,
            item: FieldItem{
                is_any: false,
                is_range: true,
                start: s as u8,
                end: e as u8,
                step: step,
            },
        };
    }

    // Single value (optionally with `/step`)
    const v = parseAtom(base_tok, min_val, max_val, allow_month, allow_dow);
    if (v < 0) return fail;
    const norm_v = if (allow_dow && v == 7) 0 as u8 else v as u8;
    return ItemParseRes{
        ok: true,
        item: FieldItem{
            is_any: false,
            is_range: false,
            start: norm_v,
            end: norm_v,
            step: step,
        },
    };
}

struct FieldParseRes {
    ok: bool,
    field: CronField,
}

fn parseField(tok: string, min_val: u8, max_val: u8, allow_month: bool, allow_dow: bool): FieldParseRes {
    let fail = FieldParseRes{ ok: false, field: CronField{} };
    if (tok.len == 0) return fail;

    let field = CronField{};
    let start: usize = 0;
    let i: usize = 0;
    let count: usize = 0;

    while (i <= tok.len) : (i += 1) {
        if (i == tok.len || tok[i] == ',') {
            if (i == start) return fail; // empty list entry like "1,,2" or ",1"
            if (count >= MAX_LIST_ITEMS) return fail;
            const part = tok[start..i];
            const parsed = parseFieldItem(part, min_val, max_val, allow_month, allow_dow);
            if (!parsed.ok) return fail;
            field.items[count] = parsed.item;
            count += 1;
            start = i + 1;
        }
    }

    if (count == 0) return fail;
    field.item_len = count as u8;

    if (count > 1) {
        field.kind = FIELD_LIST;
    } else {
        const it = field.items[0];
        if (it.step > 0) {
            field.kind = FIELD_STEP;
        } else if (it.is_any) {
            field.kind = FIELD_ANY;
        } else if (it.is_range) {
            field.kind = FIELD_RANGE;
        } else {
            field.kind = FIELD_VALUE;
        }
    }
    return FieldParseRes{ ok: true, field: field };
}

fn expandMacro(trimmed: string): string {
    if (eqIgnoreCase(trimmed, "@yearly") || eqIgnoreCase(trimmed, "@annually")) {
        return "0 0 1 1 *";
    }
    if (eqIgnoreCase(trimmed, "@monthly")) {
        return "0 0 1 * *";
    }
    if (eqIgnoreCase(trimmed, "@weekly")) {
        return "0 0 * * 0";
    }
    if (eqIgnoreCase(trimmed, "@daily") || eqIgnoreCase(trimmed, "@midnight")) {
        return "0 0 * * *";
    }
    if (eqIgnoreCase(trimmed, "@hourly")) {
        return "0 * * * *";
    }
    return "";
}

fn parseCron(raw: string): ParsedCron {
    let invalid = ParsedCron{ valid: false };
    if (raw.len == 0) return invalid;

    let start: usize = 0;
    let end: usize = raw.len;
    while (start < end && isSpace(raw[start])) {
        start += 1;
    }
    while (end > start && isSpace(raw[end - 1])) {
        end -= 1;
    }
    if (start >= end) return invalid;

    let expr = raw[start..end];
    if (expr[0] == '@') {
        const expanded = expandMacro(expr);
        if (expanded.len == 0) return invalid;
        expr = expanded;
    }

    // Split into exactly 5 whitespace-delimited tokens
    let tok_starts: [5]usize = [0, 0, 0, 0, 0];
    let tok_ends: [5]usize = [0, 0, 0, 0, 0];
    let tok_count: usize = 0;
    let i: usize = 0;
    while (i < expr.len) {
        while (i < expr.len && isSpace(expr[i])) {
            i += 1;
        }
        if (i >= expr.len) break;
        if (tok_count >= 5) return invalid; // too many fields
        const s = i;
        while (i < expr.len && !isSpace(expr[i])) {
            i += 1;
        }
        tok_starts[tok_count] = s;
        tok_ends[tok_count] = i;
        tok_count += 1;
    }

    if (tok_count != 5) return invalid;

    const f_min = parseField(expr[tok_starts[0]..tok_ends[0]], 0, 59, false, false);
    if (!f_min.ok) return invalid;
    const f_hour = parseField(expr[tok_starts[1]..tok_ends[1]], 0, 23, false, false);
    if (!f_hour.ok) return invalid;
    const f_dom = parseField(expr[tok_starts[2]..tok_ends[2]], 1, 31, false, false);
    if (!f_dom.ok) return invalid;
    const f_month = parseField(expr[tok_starts[3]..tok_ends[3]], 1, 12, true, false);
    if (!f_month.ok) return invalid;
    const f_dow = parseField(expr[tok_starts[4]..tok_ends[4]], 0, 7, false, true);
    if (!f_dow.ok) return invalid;

    return ParsedCron{
        valid: true,
        minute: f_min.field,
        hour: f_hour.field,
        dom: f_dom.field,
        month: f_month.field,
        dow: f_dow.field,
    };
}

fn writeItemNumeric(out: []u8, pos: usize, it: FieldItem, locale: Locale): usize {
    let p = pos;
    if (it.is_range) {
        p = pushU8(out, p, it.start);
        p = pushStr(out, p, if (locale == Locale.Zh) " 至 " else "-");
        p = pushU8(out, p, it.end);
    } else if (it.is_any) {
        p = pushByte(out, p, '*');
    } else {
        p = pushU8(out, p, it.start);
    }
    if (it.step > 0) {
        p = pushByte(out, p, '/');
        p = pushU8(out, p, it.step);
    }
    return p;
}

fn writeItemDay(out: []u8, pos: usize, it: FieldItem, locale: Locale): usize {
    let p = pos;
    if (it.is_range) {
        p = pushStr(out, p, dayName(it.start, locale));
        p = pushStr(out, p, if (locale == Locale.Zh) "至" else " through ");
        p = pushStr(out, p, dayName(it.end, locale));
    } else {
        p = pushStr(out, p, dayName(it.start, locale));
    }
    return p;
}

fn writeItemMonth(out: []u8, pos: usize, it: FieldItem, locale: Locale): usize {
    let p = pos;
    if (it.is_range) {
        p = pushStr(out, p, monthName(it.start, locale));
        p = pushStr(out, p, if (locale == Locale.Zh) "至" else " through ");
        p = pushStr(out, p, monthName(it.end, locale));
    } else {
        p = pushStr(out, p, monthName(it.start, locale));
    }
    return p;
}

/// Validate a standard 5-field Cron expression or predefined macro (`@daily`, `@hourly`, etc.).
pub fn isValidCron(expr: string): bool {
    const parsed = parseCron(expr);
    return parsed.valid;
}

/// Translate a Cron expression into human-readable natural language with custom `CronDescOptions`.
/// Writes directly into `out` with zero heap allocations. Returns `""` if the expression is invalid.
pub fn describeCronWith(expr: string, opts: CronDescOptions, out: []u8): string {
    const c = parseCron(expr);
    if (!c.valid) return "";

    const is_zh = opts.locale == Locale.Zh;
    const comma = if (is_zh) "，" else ", ";
    const has_date_filter = c.dom.kind != FIELD_ANY || c.month.kind != FIELD_ANY || c.dow.kind != FIELD_ANY;

    let pos: usize = 0;

    // 1. Time phrase (minute + hour)
    if (c.minute.kind == FIELD_ANY && c.hour.kind == FIELD_ANY) {
        pos = pushStr(out, pos, if (is_zh) "每分钟" else "Every minute");
    } else if (c.minute.kind == FIELD_STEP && c.minute.items[0].is_any && c.hour.kind == FIELD_ANY) {
        const s = c.minute.items[0].step;
        if (s == 1) {
            pos = pushStr(out, pos, if (is_zh) "每分钟" else "Every minute");
        } else if (is_zh) {
            pos = pushStr(out, pos, "每 ");
            pos = pushU8(out, pos, s);
            pos = pushStr(out, pos, " 分钟");
        } else {
            pos = pushStr(out, pos, "Every ");
            pos = pushU8(out, pos, s);
            pos = pushStr(out, pos, " minutes");
        }
    } else if (c.minute.kind == FIELD_VALUE && c.hour.kind == FIELD_ANY) {
        const m = c.minute.items[0].start;
        if (m == 0) {
            pos = pushStr(out, pos, if (is_zh) "每小时" else "Every hour");
        } else if (is_zh) {
            pos = pushStr(out, pos, "每小时第 ");
            pos = pushU8(out, pos, m);
            pos = pushStr(out, pos, " 分钟");
        } else {
            pos = pushStr(out, pos, "At ");
            pos = pushU8(out, pos, m);
            pos = pushStr(out, pos, " minutes past the hour");
        }
    } else if (c.minute.kind == FIELD_VALUE && c.hour.kind == FIELD_VALUE) {
        const h = c.hour.items[0].start;
        const m = c.minute.items[0].start;
        if (!has_date_filter) {
            if (is_zh) {
                pos = pushStr(out, pos, "每天 ");
                pos = pushTime(out, pos, h, m, opts);
            } else {
                pos = pushStr(out, pos, "At ");
                pos = pushTime(out, pos, h, m, opts);
                pos = pushStr(out, pos, ", every day");
            }
        } else {
            pos = pushStr(out, pos, if (is_zh) "在 " else "At ");
            pos = pushTime(out, pos, h, m, opts);
        }
    } else if (c.minute.kind == FIELD_VALUE && c.hour.kind == FIELD_STEP && c.hour.items[0].is_any) {
        const m = c.minute.items[0].start;
        const hs = c.hour.items[0].step;
        if (m == 0) {
            if (is_zh) {
                pos = pushStr(out, pos, "每 ");
                pos = pushU8(out, pos, hs);
                pos = pushStr(out, pos, " 小时");
            } else {
                pos = pushStr(out, pos, "Every ");
                pos = pushU8(out, pos, hs);
                pos = pushStr(out, pos, " hours");
            }
        } else if (is_zh) {
            pos = pushStr(out, pos, "每 ");
            pos = pushU8(out, pos, hs);
            pos = pushStr(out, pos, " 小时的第 ");
            pos = pushU8(out, pos, m);
            pos = pushStr(out, pos, " 分钟");
        } else {
            pos = pushStr(out, pos, "At ");
            pos = pushU8(out, pos, m);
            pos = pushStr(out, pos, " minutes past the hour, every ");
            pos = pushU8(out, pos, hs);
            pos = pushStr(out, pos, " hours");
        }
    } else {
        // General minute description
        if (c.minute.kind == FIELD_ANY) {
            pos = pushStr(out, pos, if (is_zh) "每分钟" else "Every minute");
        } else if (c.minute.kind == FIELD_STEP) {
            const it = c.minute.items[0];
            if (is_zh) {
                pos = pushStr(out, pos, "每 ");
                pos = pushU8(out, pos, it.step);
                pos = pushStr(out, pos, " 分钟");
            } else {
                pos = pushStr(out, pos, "Every ");
                pos = pushU8(out, pos, it.step);
                pos = pushStr(out, pos, " minutes");
            }
        } else if (c.minute.kind == FIELD_VALUE) {
            if (is_zh) {
                pos = pushStr(out, pos, "第 ");
                pos = pushU8(out, pos, c.minute.items[0].start);
                pos = pushStr(out, pos, " 分钟");
            } else {
                pos = pushStr(out, pos, "At minute ");
                pos = pushU8(out, pos, c.minute.items[0].start);
            }
        } else if (c.minute.kind == FIELD_RANGE) {
            const it = c.minute.items[0];
            if (is_zh) {
                pos = pushStr(out, pos, "第 ");
                pos = pushU8(out, pos, it.start);
                pos = pushStr(out, pos, " 至 ");
                pos = pushU8(out, pos, it.end);
                pos = pushStr(out, pos, " 分钟");
            } else {
                pos = pushStr(out, pos, "Minutes ");
                pos = pushU8(out, pos, it.start);
                pos = pushStr(out, pos, " through ");
                pos = pushU8(out, pos, it.end);
            }
        } else {
            pos = pushStr(out, pos, if (is_zh) "在第 " else "At minutes ");
            let k: usize = 0;
            while (k < (c.minute.item_len as usize)) : (k += 1) {
                if (k > 0) pos = pushStr(out, pos, if (is_zh) "、" else ", ");
                pos = writeItemNumeric(out, pos, c.minute.items[k], opts.locale);
            }
            if (is_zh) pos = pushStr(out, pos, " 分钟");
        }

        // Hour qualifier (if not `*`)
        if (c.hour.kind != FIELD_ANY) {
            pos = pushStr(out, pos, comma);
            if (c.hour.kind == FIELD_VALUE) {
                const h = c.hour.items[0].start;
                if (is_zh) {
                    pos = pushU8Pad2(out, pos, h);
                    pos = pushStr(out, pos, " 点");
                } else {
                    pos = pushStr(out, pos, "past hour ");
                    pos = pushU8(out, pos, h);
                }
            } else if (c.hour.kind == FIELD_STEP) {
                const hs = c.hour.items[0].step;
                if (is_zh) {
                    pos = pushStr(out, pos, "每 ");
                    pos = pushU8(out, pos, hs);
                    pos = pushStr(out, pos, " 小时");
                } else {
                    pos = pushStr(out, pos, "every ");
                    pos = pushU8(out, pos, hs);
                    pos = pushStr(out, pos, " hours");
                }
            } else if (c.hour.kind == FIELD_RANGE) {
                const it = c.hour.items[0];
                if (is_zh) {
                    pos = pushTime(out, pos, it.start, 0, opts);
                    pos = pushStr(out, pos, " 至 ");
                    pos = pushTime(out, pos, it.end, 59, opts);
                } else {
                    pos = pushStr(out, pos, "between ");
                    pos = pushTime(out, pos, it.start, 0, opts);
                    pos = pushStr(out, pos, " and ");
                    pos = pushTime(out, pos, it.end, 59, opts);
                }
            } else {
                pos = pushStr(out, pos, if (is_zh) "在 " else "past hours ");
                let k: usize = 0;
                while (k < (c.hour.item_len as usize)) : (k += 1) {
                    if (k > 0) pos = pushStr(out, pos, if (is_zh) "、" else ", ");
                    pos = writeItemNumeric(out, pos, c.hour.items[k], opts.locale);
                }
                if (is_zh) pos = pushStr(out, pos, " 点");
            }
        }
    }

    // 2. Day of Month (DOM)
    if (c.dom.kind != FIELD_ANY) {
        pos = pushStr(out, pos, comma);
        if (c.dom.kind == FIELD_VALUE) {
            if (is_zh) {
                pos = pushStr(out, pos, "每月第 ");
                pos = pushU8(out, pos, c.dom.items[0].start);
                pos = pushStr(out, pos, " 天");
            } else {
                pos = pushStr(out, pos, "on day ");
                pos = pushU8(out, pos, c.dom.items[0].start);
                pos = pushStr(out, pos, " of the month");
            }
        } else if (c.dom.kind == FIELD_STEP) {
            const s = c.dom.items[0].step;
            if (is_zh) {
                pos = pushStr(out, pos, "每 ");
                pos = pushU8(out, pos, s);
                pos = pushStr(out, pos, " 天");
            } else {
                pos = pushStr(out, pos, "every ");
                pos = pushU8(out, pos, s);
                pos = pushStr(out, pos, " days");
            }
        } else if (c.dom.kind == FIELD_RANGE) {
            const it = c.dom.items[0];
            if (is_zh) {
                pos = pushStr(out, pos, "每月第 ");
                pos = pushU8(out, pos, it.start);
                pos = pushStr(out, pos, " 至 ");
                pos = pushU8(out, pos, it.end);
                pos = pushStr(out, pos, " 天");
            } else {
                pos = pushStr(out, pos, "between day ");
                pos = pushU8(out, pos, it.start);
                pos = pushStr(out, pos, " and ");
                pos = pushU8(out, pos, it.end);
                pos = pushStr(out, pos, " of the month");
            }
        } else {
            pos = pushStr(out, pos, if (is_zh) "每月第 " else "on days ");
            let k: usize = 0;
            while (k < (c.dom.item_len as usize)) : (k += 1) {
                if (k > 0) pos = pushStr(out, pos, if (is_zh) "、" else ", ");
                pos = writeItemNumeric(out, pos, c.dom.items[k], opts.locale);
            }
            pos = pushStr(out, pos, if (is_zh) " 天" else " of the month");
        }
    }

    // 3. Day of Week (DOW)
    if (c.dow.kind != FIELD_ANY) {
        pos = pushStr(out, pos, comma);
        if (c.dow.kind == FIELD_VALUE) {
            pos = pushStr(out, pos, if (is_zh) "仅在" else "only on ");
            pos = pushStr(out, pos, dayName(c.dow.items[0].start, opts.locale));
        } else if (c.dow.kind == FIELD_RANGE) {
            pos = writeItemDay(out, pos, c.dow.items[0], opts.locale);
        } else if (c.dow.kind == FIELD_STEP) {
            const s = c.dow.items[0].step;
            if (is_zh) {
                pos = pushStr(out, pos, "每周每 ");
                pos = pushU8(out, pos, s);
                pos = pushStr(out, pos, " 天");
            } else {
                pos = pushStr(out, pos, "every ");
                pos = pushU8(out, pos, s);
                pos = pushStr(out, pos, " days of the week");
            }
        } else {
            pos = pushStr(out, pos, if (is_zh) "仅在" else "only on ");
            let k: usize = 0;
            while (k < (c.dow.item_len as usize)) : (k += 1) {
                if (k > 0) pos = pushStr(out, pos, if (is_zh) "、" else ", ");
                pos = writeItemDay(out, pos, c.dow.items[k], opts.locale);
            }
        }
    }

    // 4. Month
    if (c.month.kind != FIELD_ANY) {
        pos = pushStr(out, pos, comma);
        if (c.month.kind == FIELD_VALUE) {
            pos = pushStr(out, pos, if (is_zh) "仅在 " else "only in ");
            pos = pushStr(out, pos, monthName(c.month.items[0].start, opts.locale));
        } else if (c.month.kind == FIELD_RANGE) {
            pos = writeItemMonth(out, pos, c.month.items[0], opts.locale);
        } else if (c.month.kind == FIELD_STEP) {
            const s = c.month.items[0].step;
            if (is_zh) {
                pos = pushStr(out, pos, "每 ");
                pos = pushU8(out, pos, s);
                pos = pushStr(out, pos, " 个月");
            } else {
                pos = pushStr(out, pos, "every ");
                pos = pushU8(out, pos, s);
                pos = pushStr(out, pos, " months");
            }
        } else {
            pos = pushStr(out, pos, if (is_zh) "仅在 " else "only in ");
            let k: usize = 0;
            while (k < (c.month.item_len as usize)) : (k += 1) {
                if (k > 0) pos = pushStr(out, pos, if (is_zh) "、" else ", ");
                pos = writeItemMonth(out, pos, c.month.items[k], opts.locale);
            }
        }
    }

    return out[0..pos];
}

/// Translate a Cron expression into English (24-hour clock) natural language.
/// Writes directly into `out` with zero heap allocations. Returns `""` if invalid.
pub fn describeCron(expr: string, out: []u8): string {
    return describeCronWith(expr, CronDescOptions.english(), out);
}
