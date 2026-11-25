// Official @web/env Plugin for `cap:web`
// Startup environment schema validation (required, default, int range, bool, enum)
// and automatic App.setState injection (inspired by @fastify/env).

import { App } from "../../../../lib/web/app.cp";

pub const ENV_MAX_SPECS: usize = 32;

pub enum EnvKind {
    Str,
    Int,
    Bool,
    Enum,
}

pub struct EnvVarSpec {
    name: string = "";
    kind: EnvKind = EnvKind.Str;
    required: bool = false;
    default_val: string = "";
    min_val: i64 = 0;
    max_val: i64 = 0;
    has_bounds: bool = false;
    enum_values: string = "";
}

pub struct EnvKV {
    key: string = "";
    value: string = "";
}

pub struct EnvValidationResult {
    ok: bool = true,
    missing_count: usize = 0,
    invalid_count: usize = 0,
    loaded_count: usize = 0,
    first_error_key: string = "",
}

fn envStrEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn envParseI64(s: string): i64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
        if (i >= s.len) return null;
    }
    let acc: i64 = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        acc = acc * 10 + ((c - '0') as i64);
    }
    return if (neg) -acc else acc;
}

fn envIsValidBool(s: string): bool {
    return envStrEql(s, "true") || envStrEql(s, "false") || envStrEql(s, "1") || envStrEql(s, "0");
}

fn envInEnum(val: string, list: string): bool {
    if (list.len == 0) return false;
    let pos: usize = 0;
    while (pos <= list.len) {
        let end = pos;
        while (end < list.len && list[end] != ',') : (end += 1) {}
        if (envStrEql(val, list[pos..end])) return true;
        if (end >= list.len) break;
        pos = end + 1;
    }
    return false;
}

fn envLookupRaw(entries: []const EnvKV, key: string): string? {
    let i: usize = 0;
    while (i < entries.len) : (i += 1) {
        if (envStrEql(entries[i].key, key)) {
            return entries[i].value;
        }
    }
    return null;
}

/// Validate `entries` against `specs` and inject resolved values into `app.setState(key, val)`.
pub fn registerEnv(app: *App, specs: []const EnvVarSpec, entries: []const EnvKV): EnvValidationResult {
    let res = EnvValidationResult{};
    let i: usize = 0;
    while (i < specs.len && i < ENV_MAX_SPECS) : (i += 1) {
        const sp = specs[i];
        let val = envLookupRaw(entries, sp.name) ?? "";
        if (val.len == 0) {
            if (sp.default_val.len > 0) {
                val = sp.default_val;
            } else if (sp.required) {
                res.ok = false;
                res.missing_count += 1;
                if (res.first_error_key.len == 0) res.first_error_key = sp.name;
                continue;
            } else {
                continue;
            }
        }

        let valid = true;
        switch (sp.kind) {
            EnvKind.Str => {},
            EnvKind.Int => {
                if (n := envParseI64(val)) {
                    if (sp.has_bounds && (n < sp.min_val || n > sp.max_val)) {
                        valid = false;
                    }
                } else {
                    valid = false;
                }
            },
            EnvKind.Bool => {
                if (!envIsValidBool(val)) valid = false;
            },
            EnvKind.Enum => {
                if (!envInEnum(val, sp.enum_values)) valid = false;
            },
        }

        if (!valid) {
            res.ok = false;
            res.invalid_count += 1;
            if (res.first_error_key.len == 0) res.first_error_key = sp.name;
            continue;
        }

        * = app.setState(sp.name, val);
        res.loaded_count += 1;
    }
    return res;
}
