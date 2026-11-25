// Official @web/flags Plugin for `cap:web`
// Zero-heap Feature Flags with deterministic FNV-1a percentage bucketing,
// allowlist targeting, and `GET /_flags` status inspection.

import {
    App,
    AppMethod,
    Context,
    Response,
    Plugin,
    PluginScope,
} from "../../../../lib/web/app.cp";

pub const FLAGS_MAX_ITEMS: usize = 32;
pub const FLAGS_JSON_BUF_CAP: usize = 4096;

pub struct FlagsConfig {
    status_path: string = "/_flags";
    user_header: string = "X-User-Id";
}

pub struct FeatureFlag {
    name: string = "";
    enabled: bool = false;
    rollout_pct: u8 = 0; // 0..=100
    allow_users: string = ""; // comma-separated user IDs
}

let g_flg_cfg: FlagsConfig = FlagsConfig{};
let g_flg_items: [FLAGS_MAX_ITEMS]FeatureFlag = [FeatureFlag{}; FLAGS_MAX_ITEMS];
let g_flg_count: usize = 0;
let g_flg_json_buf: [FLAGS_JSON_BUF_CAP]u8 = undefined;

fn flgStrEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn flgInList(list: string, target: string): bool {
    if (list.len == 0 || target.len == 0) return false;
    let pos: usize = 0;
    while (pos <= list.len) {
        let end = pos;
        while (end < list.len && list[end] != ',') : (end += 1) {}
        if (flgStrEql(list[pos..end], target)) return true;
        if (end >= list.len) break;
        pos = end + 1;
    }
    return false;
}

/// Deterministic FNV-1a bucket in `[0, 99]` for consistent percentage rollouts.
pub fn flagBucket(flag_name: string, user_key: string): u8 {
    let h: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
    let i: usize = 0;
    while (i < flag_name.len) : (i += 1) {
        h = (h ^ (flag_name[i] as u64)) *% 1099511628211;
    }
    h = (h ^ (':' as u64)) *% 1099511628211;
    i = 0;
    while (i < user_key.len) : (i += 1) {
        h = (h ^ (user_key[i] as u64)) *% 1099511628211;
    }
    return (h % 100) as u8;
}

pub fn flagReset() {
    g_flg_count = 0;
}

pub fn flagDefine(name: string, enabled: bool, rollout_pct: u8, allow_users: string): bool {
    let i: usize = 0;
    while (i < g_flg_count) : (i += 1) {
        if (flgStrEql(g_flg_items[i].name, name)) {
            g_flg_items[i] = FeatureFlag{
                name: name,
                enabled: enabled,
                rollout_pct: rollout_pct,
                allow_users: allow_users,
            };
            return true;
        }
    }
    if (g_flg_count >= FLAGS_MAX_ITEMS) return false;
    g_flg_items[g_flg_count] = FeatureFlag{
        name: name,
        enabled: enabled,
        rollout_pct: rollout_pct,
        allow_users: allow_users,
    };
    g_flg_count += 1;
    return true;
}

/// Evaluate whether `flag_name` is active for `user_key`.
pub fn flagEval(flag_name: string, user_key: string): bool {
    let i: usize = 0;
    while (i < g_flg_count) : (i += 1) {
        const f = g_flg_items[i];
        if (!flgStrEql(f.name, flag_name)) continue;
        if (!f.enabled) return false;
        if (flgInList(f.allow_users, user_key)) return true;
        if (f.rollout_pct >= 100) return true;
        if (f.rollout_pct == 0) return false;
        if (user_key.len == 0) return false;
        return flagBucket(flag_name, user_key) < f.rollout_pct;
    }
    return false;
}

fn flgWriteRaw(dst: []u8, pos: *usize, s: string): bool {
    if (s.len > dst.len || pos.* > dst.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        dst[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn flgWriteU8(dst: []u8, pos: *usize, v: u8): bool {
    let tmp: [4]u8 = undefined;
    let n: usize = 0;
    let x = v;
    if (x == 0) {
        return flgWriteRaw(dst, pos, "0");
    }
    while (x > 0) {
        tmp[n] = (48 + (x % 10)) as u8;
        n += 1;
        x = x / 10;
    }
    if (pos.* + n > dst.len) return false;
    while (n > 0) {
        n -= 1;
        dst[pos.*] = tmp[n];
        pos.* += 1;
    }
    return true;
}

fn handleFlagsStatus(ctx: *Context): Response {
    const uid = ctx.header(g_flg_cfg.user_header) ?? "";
    let pos: usize = 0;
    * = flgWriteRaw(g_flg_json_buf[0..], *pos, "{\"flags\":[");
    let i: usize = 0;
    while (i < g_flg_count) : (i += 1) {
        const f = g_flg_items[i];
        if (i > 0) {
            * = flgWriteRaw(g_flg_json_buf[0..], *pos, ",");
        }
        * = flgWriteRaw(g_flg_json_buf[0..], *pos, "{\"name\":\"");
        * = flgWriteRaw(g_flg_json_buf[0..], *pos, f.name);
        * = flgWriteRaw(g_flg_json_buf[0..], *pos, "\",\"rollout\":");
        * = flgWriteU8(g_flg_json_buf[0..], *pos, f.rollout_pct);
        * = flgWriteRaw(g_flg_json_buf[0..], *pos, ",\"active\":");
        if (flagEval(f.name, uid)) {
            * = flgWriteRaw(g_flg_json_buf[0..], *pos, "true}");
        } else {
            * = flgWriteRaw(g_flg_json_buf[0..], *pos, "false}");
        }
    }
    * = flgWriteRaw(g_flg_json_buf[0..], *pos, "]}");
    return Response.json(200, g_flg_json_buf[0..pos]);
}

pub fn createFlagsPlugin(cfg: FlagsConfig): Plugin {
    g_flg_cfg = cfg;
    let p = Plugin.init("@web/flags");
    * = p.withScope(PluginScope.Global);
    if (cfg.status_path.len > 0) {
        * = p.route(AppMethod.GET, cfg.status_path, "Feature Flags Status", handleFlagsStatus);
    }
    return p;
}

pub fn registerFlags(app: *App, cfg: FlagsConfig): *App {
    const p = createFlagsPlugin(cfg);
    return app.usePlugin(&p);
}
