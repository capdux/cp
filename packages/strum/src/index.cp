// @cap/strum-enum — Zero-heap enum variant registry, discriminant lookup & case-insensitive parser.
// Clean-room MIT port of Rust's `strum` crate (900M+ downloads); reuses `@cap/unicase`.

import { eqAscii } from "../../unicase/src/index.cp";

pub const STRUM_MAX_VARIANTS: usize = 32;

pub struct EnumVariant {
    pub id: i32,
    pub name: string,
    pub alias: string,
    pub message: string,
}

fn stmStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

/// Zero-heap enum metadata registry (`strum::EnumString` + `AsRef<str>` + `EnumMessage` + `EnumCount`).
pub struct EnumRegistry {
    pub variants: [32]EnumVariant,
    pub count: usize,

    pub static fn new(): EnumRegistry {
        let empty = EnumVariant { id: 0, name: "", alias: "", message: "" };
        let arr: [32]EnumVariant = undefined;
        let i: usize = 0;
        while (i < STRUM_MAX_VARIANTS) { arr[i] = empty; i = i + 1; }
        return EnumRegistry { variants: arr, count: 0 };
    }

    pub fn variantCount(): usize { return _.count; }

    pub fn register(*_, id: i32, name: string, alias: string, message: string): bool {
        if (_.count >= STRUM_MAX_VARIANTS) { return false; }
        _.variants[_.count] = EnumVariant { id: id, name: name, alias: alias, message: message };
        _.count = _.count + 1;
        return true;
    }

    /// Exact case-sensitive `FromStr` lookup by canonical name or alias.
    pub fn fromStr(s: string): i32? {
        let i: usize = 0;
        while (i < _.count) {
            if (stmStrEq(_.variants[i].name, s) || (_.variants[i].alias.len > 0 && stmStrEq(_.variants[i].alias, s))) {
                return _.variants[i].id;
            }
            i = i + 1;
        }
        return null;
    }

    /// ASCII case-insensitive `FromStr` lookup via `@cap/unicase`.
    pub fn fromStrIgnoreCase(s: string): i32? {
        let i: usize = 0;
        while (i < _.count) {
            if (eqAscii(_.variants[i].name, s) || (_.variants[i].alias.len > 0 && eqAscii(_.variants[i].alias, s))) {
                return _.variants[i].id;
            }
            i = i + 1;
        }
        return null;
    }

    /// `AsRef<str>` — returns canonical string name for discriminant `id`.
    pub fn asStr(id: i32): string? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.variants[i].id == id) { return _.variants[i].name; }
            i = i + 1;
        }
        return null;
    }

    /// `EnumMessage` — returns documentation/message for discriminant `id`.
    pub fn getMessage(id: i32): string? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.variants[i].id == id) { return _.variants[i].message; }
            i = i + 1;
        }
        return null;
    }

    /// Returns the `idx`-th registered variant discriminant for `EnumIter`-style iteration.
    pub fn variantAt(idx: usize): EnumVariant? {
        if (idx >= _.count) { return null; }
        return _.variants[idx];
    }

    /// Formats all variant names separated by `", "` into `out` (useful for CLI help / error hints).
    pub fn formatVariantList(out: []u8): usize {
        let p: usize = 0;
        let i: usize = 0;
        while (i < _.count) {
            if (i > 0) {
                if (p + 2 <= out.len) {
                    out[p] = 44; // ','
                    out[p + 1] = 32; // ' '
                    p = p + 2;
                }
            }
            let name = _.variants[i].name;
            let k: usize = 0;
            while (k < name.len && p < out.len) {
                out[p] = name[k];
                p = p + 1;
                k = k + 1;
            }
            i = i + 1;
        }
        return p;
    }
}
