// @cap/y18n — Zero-allocation internationalization (i18n) engine in pure CP
// Clean-room MIT implementation inspired by npm `y18n` (yargs i18n engine).
// Supports fixed-capacity translation catalogs, singular/plural lookup (`t` / `tn`),
// locale switching, and zero-heap `%s` / `%d` / `%%` printf formatting.

import { strEq } from "cap:ascii";
import { writeStr, writeI64 } from "cap:fmt";

pub const Y18_MAX_ENTRIES: usize = 64;

pub struct Y18nEntry {
    pub key: string,
    pub singular: string,
    pub plural: string,
    pub has_plural: bool,
}

fn y18WriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

/// Standalone zero-allocation printf-style formatter supporting `%s`, `%d`, and `%%`.
/// When `has_count` is true, `%d` injects `count`; otherwise `%d` consumes the next positional arg.
/// Unmatched `%s`/`%d` placeholders (when `args` is exhausted) are preserved literally.
/// Writes safely within `out.len` and returns the number of bytes written.
pub fn formatPrintf(tmpl: string, count: i64, has_count: bool, args: []const string, out: []u8): usize {
    if (out.len == 0 || tmpl.len == 0) {
        return 0;
    }
    let w: usize = 0;
    let i: usize = 0;
    let arg_idx: usize = 0;

    while (i < tmpl.len && w < out.len) {
        const ch: u8 = tmpl[i];
        if (ch == 37 && i + 1 < tmpl.len) { // '%'
            const next: u8 = tmpl[i + 1];
            if (next == 37) { // "%%"
                w = y18WriteByte(out, w, 37);
                i += 2;
                continue;
            }
            if (next == 115) { // "%s"
                if (arg_idx < args.len) {
                    w = writeStr(args[arg_idx], out, w);
                    arg_idx += 1;
                } else {
                    w = writeStr("%s", out, w);
                }
                i += 2;
                continue;
            }
            if (next == 100) { // "%d"
                if (has_count) {
                    w = writeI64(count, out, w);
                } else if (arg_idx < args.len) {
                    w = writeStr(args[arg_idx], out, w);
                    arg_idx += 1;
                } else {
                    w = writeStr("%d", out, w);
                }
                i += 2;
                continue;
            }
        }
        w = y18WriteByte(out, w, ch);
        i += 1;
    }
    return w;
}

pub struct Y18nCatalog {
    pub locale: string,
    pub count: usize,
    entries: [64]Y18nEntry,

    pub static fn new(locale: string): Y18nCatalog {
        let arr: [64]Y18nEntry = undefined;
        return Y18nCatalog {
            locale: locale,
            count: 0,
            entries: arr,
        };
    }

    pub fn setLocale(*_, locale: string) {
        _.locale = locale;
    }

    pub fn getLocale(): string {
        return _.locale;
    }

    pub fn clear(*_) {
        _.count = 0;
    }

    pub fn findIndex(key: string): i64 {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].key, key)) {
                return i as i64;
            }
            i += 1;
        }
        return -1;
    }

    pub fn hasEntry(key: string): bool {
        return _.findIndex(key) >= 0;
    }

    /// Insert or update a singular translation entry. Returns false if catalog is full.
    pub fn setEntry(*_, key: string, translation: string): bool {
        const idx: i64 = _.findIndex(key);
        if (idx >= 0) {
            const u: usize = idx as usize;
            _.entries[u].singular = translation;
            return true;
        }
        if (_.count >= Y18_MAX_ENTRIES) {
            return false;
        }
        _.entries[_.count] = Y18nEntry {
            key: key,
            singular: translation,
            plural: "",
            has_plural: false,
        };
        _.count += 1;
        return true;
    }

    /// Insert or update a singular + plural translation entry keyed by `singular_key`.
    pub fn setPluralEntry(*_, singular_key: string, singular_tr: string, plural_tr: string): bool {
        const idx: i64 = _.findIndex(singular_key);
        if (idx >= 0) {
            const u: usize = idx as usize;
            _.entries[u].singular = singular_tr;
            _.entries[u].plural = plural_tr;
            _.entries[u].has_plural = true;
            return true;
        }
        if (_.count >= Y18_MAX_ENTRIES) {
            return false;
        }
        _.entries[_.count] = Y18nEntry {
            key: singular_key,
            singular: singular_tr,
            plural: plural_tr,
            has_plural: true,
        };
        _.count += 1;
        return true;
    }

    /// Translate `key` with optional `%s` positional arguments (equivalent to `y18n.__`).
    /// Falls back to `key` itself if no translation entry exists.
    pub fn t(key: string, args: []const string, out: []u8): usize {
        let tmpl: string = key;
        const idx: i64 = _.findIndex(key);
        if (idx >= 0) {
            tmpl = _.entries[idx as usize].singular;
        }
        return formatPrintf(tmpl, 0, false, args, out);
    }

    /// Translate singular/plural message with `count` (`%d`) and optional `%s` arguments
    /// (equivalent to `y18n.__n`). Uses singular template when `count == 1`, otherwise plural.
    pub fn tn(singular: string, plural: string, count: i64, args: []const string, out: []u8): usize {
        let tmpl: string = plural;
        if (count == 1) {
            tmpl = singular;
        }
        const idx: i64 = _.findIndex(singular);
        if (idx >= 0) {
            const entry = _.entries[idx as usize];
            if (count == 1) {
                tmpl = entry.singular;
            } else if (entry.has_plural) {
                tmpl = entry.plural;
            } else {
                tmpl = entry.singular;
            }
        }
        return formatPrintf(tmpl, count, true, args, out);
    }
}

export {
    Y18_MAX_ENTRIES,
    Y18nEntry,
    Y18nCatalog,
    formatPrintf,
};
