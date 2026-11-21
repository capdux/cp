// Source: Absorbed from /data/dev/bun/src/env_loader.zig (XDG Base Directory Spec Resolver)

pub const DEFAULT_CONFIG_DIRS: []const u8 = "/etc/xdg";
pub const DEFAULT_DATA_DIRS: []const u8 = "/usr/local/share:/usr/share";

pub struct XdgEnv {
    home: ([]const u8)?;
    config_home: ([]const u8)?;
    data_home: ([]const u8)?;
    cache_home: ([]const u8)?;
    state_home: ([]const u8)?;
    runtime_dir: ([]const u8)?;
    config_dirs: ([]const u8)?;
    data_dirs: ([]const u8)?;

    fn empty(home: ([]const u8)?): XdgEnv {
        return XdgEnv {
            home: home,
            config_home: null,
            data_home: null,
            cache_home: null,
            state_home: null,
            runtime_dir: null,
            config_dirs: null,
            data_dirs: null,
        };
    }
}

fn isAbsPath(p: []const u8): bool {
    if (p.len == 0 || p[0] != '/') {
        return false;
    }
    let i: usize = 0;
    while (i < p.len) {
        if (p[i] == 0) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimTrailingSlashes(p: []const u8): []const u8 {
    let end = p.len;
    while (end > 1 && p[end - 1] == '/') {
        end -= 1;
    }
    return p[0..end];
}

fn isSafeAppSubpath(app: []const u8): bool {
    if (app.len == 0) {
        return true;
    }
    if (app[0] == '/') {
        return false;
    }
    let i: usize = 0;
    while (i < app.len) {
        if (app[i] == 0) {
            return false;
        }
        const seg_start = i;
        while (i < app.len && app[i] != '/') {
            if (app[i] == 0) {
                return false;
            }
            i += 1;
        }
        const seg_len = i - seg_start;
        if (seg_len == 2 && app[seg_start] == '.' && app[seg_start + 1] == '.') {
            return false;
        }
        if (i < app.len && app[i] == '/') {
            i += 1;
        }
    }
    return true;
}

fn copySlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* > out.len || s.len > out.len - pos.*) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn copyByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) {
        return false;
    }
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn joinBaseAndApp(base: []const u8, suffix: []const u8, app_name: ([]const u8)?, out: []u8): ([]const u8)? {
    const clean_base = trimTrailingSlashes(base);
    let pos: usize = 0;

    // 避免根目录 "/" 拼接 "/.config" 产生 "//.config"
    if (clean_base.len == 1 && clean_base[0] == '/' && suffix.len > 0 && suffix[0] == '/') {
        if (!copySlice(out, *pos, suffix)) {
            return null;
        }
    } else {
        if (!copySlice(out, *pos, clean_base)) {
            return null;
        }
        if (suffix.len > 0) {
            if (!copySlice(out, *pos, suffix)) {
                return null;
            }
        }
    }

    if (raw_app := app_name) {
        const clean_app = trimTrailingSlashes(raw_app);
        if (clean_app.len > 0) {
            if (!isSafeAppSubpath(clean_app)) {
                return null;
            }
            if (pos == 0 || out[pos - 1] != '/') {
                if (!copyByte(out, *pos, '/')) {
                    return null;
                }
            }
            if (!copySlice(out, *pos, clean_app)) {
                return null;
            }
        }
    }

    return out[0..pos];
}

fn resolveWithFallback(
    override_dir: ([]const u8)?,
    home: ([]const u8)?,
    home_suffix: []const u8,
    app_name: ([]const u8)?,
    out: []u8,
): ([]const u8)? {
    if (ov := override_dir) {
        if (isAbsPath(ov)) {
            return joinBaseAndApp(ov, "", app_name, out);
        }
    }
    if (h := home) {
        if (isAbsPath(h)) {
            return joinBaseAndApp(h, home_suffix, app_name, out);
        }
    }
    return null;
}

pub fn resolveConfigHome(env: XdgEnv, app_name: ([]const u8)?, out: []u8): ([]const u8)? {
    return resolveWithFallback(env.config_home, env.home, "/.config", app_name, out);
}

pub fn resolveDataHome(env: XdgEnv, app_name: ([]const u8)?, out: []u8): ([]const u8)? {
    return resolveWithFallback(env.data_home, env.home, "/.local/share", app_name, out);
}

pub fn resolveCacheHome(env: XdgEnv, app_name: ([]const u8)?, out: []u8): ([]const u8)? {
    return resolveWithFallback(env.cache_home, env.home, "/.cache", app_name, out);
}

pub fn resolveStateHome(env: XdgEnv, app_name: ([]const u8)?, out: []u8): ([]const u8)? {
    return resolveWithFallback(env.state_home, env.home, "/.local/state", app_name, out);
}

pub fn resolveRuntimeDir(env: XdgEnv, app_name: ([]const u8)?, out: []u8): ([]const u8)? {
    if (rt := env.runtime_dir) {
        if (isAbsPath(rt)) {
            return joinBaseAndApp(rt, "", app_name, out);
        }
    }
    return null;
}

fn hasAnyAbsDir(list: []const u8): bool {
    let i: usize = 0;
    while (i < list.len) {
        const start = i;
        while (i < list.len && list[i] != ':') {
            i += 1;
        }
        if (isAbsPath(list[start..i])) {
            return true;
        }
        if (i < list.len && list[i] == ':') {
            i += 1;
        }
    }
    return false;
}

pub struct XdgDirIter {
    src: []const u8;
    pos: usize;

    fn init(dirs_env: ([]const u8)?, default_dirs: []const u8): XdgDirIter {
        let chosen = default_dirs;
        if (raw := dirs_env) {
            if (raw.len > 0 && hasAnyAbsDir(raw)) {
                chosen = raw;
            }
        }
        return XdgDirIter {
            src: chosen,
            pos: 0,
        };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ':') {
                _.pos += 1;
            }
            let end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == ':') {
                _.pos += 1;
            }
            if (isAbsPath(_.src[start..end])) {
                while (end > start + 1 && _.src[end - 1] == '/') {
                    end -= 1;
                }
                return _.src[start..end];
            }
        }
        return null;
    }
}
