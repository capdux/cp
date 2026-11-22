// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T13:15:07Z (W3C Fetch Metadata Sec-Fetch-* Resource Isolation Policy)
// Pure-CP zero-allocation W3C Fetch Metadata parser (Sec-Fetch-Site/Mode/Dest/User) and resource-isolation guard.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) start += 1;
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) end -= 1;
    return s[start..end];
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

export enum FetchSite {
    same_origin,
    same_site,
    cross_site,
    none,
    unknown,
}

export enum FetchMode {
    cors,
    navigate,
    no_cors,
    same_origin,
    websocket,
    unknown,
}

export enum FetchDest {
    empty,
    document,
    iframe,
    script,
    style,
    image,
    font,
    audio,
    video,
    worker,
    sharedworker,
    serviceworker,
    object,
    embed,
    manifest,
    unknown,
}

export fn parseFetchSite(raw: []const u8): FetchSite {
    const s = trimWs(raw);
    if (eqIgnoreCase(s, "same-origin")) return FetchSite.same_origin;
    if (eqIgnoreCase(s, "same-site")) return FetchSite.same_site;
    if (eqIgnoreCase(s, "cross-site")) return FetchSite.cross_site;
    if (eqIgnoreCase(s, "none")) return FetchSite.none;
    return FetchSite.unknown;
}

export fn parseFetchMode(raw: []const u8): FetchMode {
    const s = trimWs(raw);
    if (eqIgnoreCase(s, "cors")) return FetchMode.cors;
    if (eqIgnoreCase(s, "navigate")) return FetchMode.navigate;
    if (eqIgnoreCase(s, "no-cors")) return FetchMode.no_cors;
    if (eqIgnoreCase(s, "same-origin")) return FetchMode.same_origin;
    if (eqIgnoreCase(s, "websocket")) return FetchMode.websocket;
    return FetchMode.unknown;
}

export fn parseFetchDest(raw: []const u8): FetchDest {
    const s = trimWs(raw);
    if (eqIgnoreCase(s, "empty")) return FetchDest.empty;
    if (eqIgnoreCase(s, "document")) return FetchDest.document;
    if (eqIgnoreCase(s, "iframe")) return FetchDest.iframe;
    if (eqIgnoreCase(s, "script")) return FetchDest.script;
    if (eqIgnoreCase(s, "style")) return FetchDest.style;
    if (eqIgnoreCase(s, "image")) return FetchDest.image;
    if (eqIgnoreCase(s, "font")) return FetchDest.font;
    if (eqIgnoreCase(s, "audio")) return FetchDest.audio;
    if (eqIgnoreCase(s, "video")) return FetchDest.video;
    if (eqIgnoreCase(s, "worker")) return FetchDest.worker;
    if (eqIgnoreCase(s, "sharedworker")) return FetchDest.sharedworker;
    if (eqIgnoreCase(s, "serviceworker")) return FetchDest.serviceworker;
    if (eqIgnoreCase(s, "object")) return FetchDest.object;
    if (eqIgnoreCase(s, "embed")) return FetchDest.embed;
    if (eqIgnoreCase(s, "manifest")) return FetchDest.manifest;
    return FetchDest.unknown;
}

export struct FetchMetadata {
    site: FetchSite?,
    mode: FetchMode?,
    dest: FetchDest?,
    user_activated: bool,

    pub fn parse(
        sec_fetch_site: ([]const u8)?,
        sec_fetch_mode: ([]const u8)?,
        sec_fetch_dest: ([]const u8)?,
        sec_fetch_user: ([]const u8)?,
    ): FetchMetadata {
        let site: FetchSite? = null;
        if (s := sec_fetch_site) site = parseFetchSite(s);
        let mode: FetchMode? = null;
        if (m := sec_fetch_mode) mode = parseFetchMode(m);
        let dest: FetchDest? = null;
        if (d := sec_fetch_dest) dest = parseFetchDest(d);
        let user = false;
        if (u := sec_fetch_user) {
            user = eqIgnoreCase(trimWs(u), "?1");
        }
        return FetchMetadata {
            site: site,
            mode: mode,
            dest: dest,
            user_activated: user,
        };
    }

    /// Standard W3C Resource Isolation Policy (web.dev/articles/fetch-metadata):
    /// 1. Allow legacy clients without `Sec-Fetch-Site`.
    /// 2. Allow `same-origin`, `same-site`, or `none` (user-initiated address bar/bookmark).
    /// 3. Allow top-level simple GET navigations (`mode == navigate` and `dest` is not `object`/`embed`).
    /// 4. Reject all other cross-site requests (e.g. cross-site POST/XHR/iframe).
    pub fn allowByResourceIsolation(is_get_or_head: bool): bool {
        const s = _.site ?? return true; // legacy client without Fetch Metadata
        if (s == FetchSite.same_origin || s == FetchSite.same_site || s == FetchSite.none) {
            return true;
        }
        if (is_get_or_head) {
            if (m := _.mode) {
                if (m == FetchMode.navigate) {
                    if (d := _.dest) {
                        if (d == FetchDest.object || d == FetchDest.embed) return false;
                    }
                    return true;
                }
            }
        }
        return false;
    }

    /// Strict API endpoint guard: only allow `same-origin` or `none` (or missing header if `allow_legacy`).
    pub fn allowSameOriginApi(allow_legacy: bool): bool {
        if (s := _.site) {
            return s == FetchSite.same_origin || s == FetchSite.none;
        }
        return allow_legacy;
    }
}
