// Official @web/i18n Plugin for `cap:web`
// Zero-heap locale negotiation (?lang= > Cookie > Accept-Language) + translation dictionary
// + automatic `Content-Language` response header (inspired by @intlify/elysia).

import {
    App,
    AppMethod,
    Context,
    Response,
    Plugin,
    PluginScope,
} from "../../../../lib/web/app.cp";

pub const I18N_MAX_LOCALES: usize = 8;
pub const I18N_MAX_ENTRIES: usize = 64;

pub struct I18nConfig {
    default_locale: string = "en";
    supported_locales: string = "en,zh,ja,fr,de,es";
    query_param: string = "lang";
    cookie_name: string = "lang";
}

struct I18nEntry {
    locale: string = "";
    key: string = "";
    val: string = "";
}

let g_i18n_cfg: I18nConfig = I18nConfig{};
let g_i18n_entries: [I18N_MAX_ENTRIES]I18nEntry = [I18nEntry{}; I18N_MAX_ENTRIES];
let g_i18n_count: usize = 0;

fn i18nAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn i18nEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (i18nAsciiLower(a[i]) != i18nAsciiLower(b[i])) return false;
    }
    return true;
}

fn i18nStrEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Match a candidate language tag (e.g. `zh-CN` or `en-US`) against comma-separated `supported`.
pub fn i18nMatchLocale(supported: string, candidate: string): string? {
    if (candidate.len == 0) return null;
    // Strip optional `;q=...`
    let end = candidate.len;
    let i: usize = 0;
    while (i < candidate.len) : (i += 1) {
        if (candidate[i] == ';') {
            end = i;
            break;
        }
    }
    let start: usize = 0;
    while (start < end && (candidate[start] == ' ' || candidate[start] == '\t')) : (start += 1) {}
    while (end > start && (candidate[end - 1] == ' ' || candidate[end - 1] == '\t')) : (end -= 1) {}
    if (end <= start) return null;
    const tag = candidate[start..end];

    // Primary subtag before '-' (e.g. "zh-CN" -> "zh")
    let dash: usize = tag.len;
    let di: usize = 0;
    while (di < tag.len) : (di += 1) {
        if (tag[di] == '-') {
            dash = di;
            break;
        }
    }
    const primary = tag[0..dash];

    let pos: usize = 0;
    while (pos <= supported.len && supported.len > 0) {
        let sep = pos;
        while (sep < supported.len && supported[sep] != ',') : (sep += 1) {}
        const sup = supported[pos..sep];
        if (i18nEqlIgnoreCase(sup, tag) || i18nEqlIgnoreCase(sup, primary)) {
            return sup;
        }
        if (sep >= supported.len) break;
        pos = sep + 1;
    }
    return null;
}

/// Negotiate best locale from `Accept-Language` header.
pub fn i18nNegotiateHeader(supported: string, accept_lang: string, fallback: string): string {
    let pos: usize = 0;
    while (pos < accept_lang.len) {
        let end = pos;
        while (end < accept_lang.len && accept_lang[end] != ',') : (end += 1) {}
        if (m := i18nMatchLocale(supported, accept_lang[pos..end])) {
            return m;
        }
        if (end >= accept_lang.len) break;
        pos = end + 1;
    }
    return fallback;
}

pub fn i18nResetCatalog() {
    g_i18n_count = 0;
}

pub fn i18nAddTranslation(locale: string, key: string, value: string): bool {
    let i: usize = 0;
    while (i < g_i18n_count) : (i += 1) {
        if (i18nStrEql(g_i18n_entries[i].locale, locale) && i18nStrEql(g_i18n_entries[i].key, key)) {
            g_i18n_entries[i].val = value;
            return true;
        }
    }
    if (g_i18n_count >= I18N_MAX_ENTRIES) return false;
    g_i18n_entries[g_i18n_count] = I18nEntry{ locale: locale, key: key, val: value };
    g_i18n_count += 1;
    return true;
}

/// Translate `key` for `locale` with fallback to `default_locale`, then raw `key`.
pub fn i18nTranslate(locale: string, key: string): string {
    let i: usize = 0;
    while (i < g_i18n_count) : (i += 1) {
        if (i18nStrEql(g_i18n_entries[i].locale, locale) && i18nStrEql(g_i18n_entries[i].key, key)) {
            return g_i18n_entries[i].val;
        }
    }
    i = 0;
    while (i < g_i18n_count) : (i += 1) {
        if (i18nStrEql(g_i18n_entries[i].locale, g_i18n_cfg.default_locale) && i18nStrEql(g_i18n_entries[i].key, key)) {
            return g_i18n_entries[i].val;
        }
    }
    return key;
}

pub fn i18nDeriveHook(ctx: *Context): bool {
    const cfg = g_i18n_cfg;
    let chosen = cfg.default_locale;

    if (q := ctx.query(cfg.query_param)) {
        if (m := i18nMatchLocale(cfg.supported_locales, q)) {
            chosen = m;
        }
    } else if (ck := ctx.cookie(cfg.cookie_name)) {
        if (m := i18nMatchLocale(cfg.supported_locales, ck)) {
            chosen = m;
        }
    } else if (al := ctx.header("Accept-Language")) {
        chosen = i18nNegotiateHeader(cfg.supported_locales, al, cfg.default_locale);
    }

    * = ctx.setDerived("locale", chosen);
    * = ctx.setHeader("Content-Language", chosen);
    return true;
}

pub fn createI18nPlugin(cfg: I18nConfig): Plugin {
    g_i18n_cfg = cfg;
    let p = Plugin.init("@web/i18n");
    * = p.withScope(PluginScope.Global).derive(i18nDeriveHook);
    return p;
}

pub fn registerI18n(app: *App, cfg: I18nConfig): *App {
    const p = createI18nPlugin(cfg);
    return app.usePlugin(&p);
}
