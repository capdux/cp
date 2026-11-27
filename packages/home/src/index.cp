// @cap/home-dir — Zero-heap cross-platform home directory & tilde path utilities in pure CP
// MIT License — Clean-room implementation inspired by Rust `home` & npm `untildify`/`tildify`

import { startsWithBytes } from "cap:bin";
import { writeStr } from "cap:fmt";

fn isSlash(ch: u8): bool {
    return ch == 47 || ch == 92; // '/' or '\\'
}

fn writeByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) {
        return pos;
    }
    out[pos] = b;
    return pos + 1;
}

fn trimTrailingSlashes(s: string): usize {
    let end: usize = s.len;
    while (end > 1 && isSlash(s[end - 1])) {
        end -= 1;
    }
    return end;
}

/// Resolve the user's home directory from environment variables into `out`:
/// - POSIX (`is_windows == false`): uses `HOME`
/// - Windows (`is_windows == true`): prefers `HOME` -> `USERPROFILE` -> `HOMEDRIVE + HOMEPATH`
/// Returns bytes written to `out`, or `0` if unresolved or `out` is empty.
pub fn resolveHomeDir(
    home_env: string,
    userprofile_env: string,
    homedrive_env: string,
    homepath_env: string,
    is_windows: bool,
    out: []u8,
): usize {
    if (out.len == 0) {
        return 0;
    }
    if (!is_windows) {
        if (home_env.len == 0) {
            return 0;
        }
        return writeStr(home_env, out, 0);
    }
    if (home_env.len > 0) {
        return writeStr(home_env, out, 0);
    }
    if (userprofile_env.len > 0) {
        return writeStr(userprofile_env, out, 0);
    }
    if (homedrive_env.len > 0 && homepath_env.len > 0) {
        let p: usize = writeStr(homedrive_env, out, 0);
        p = writeStr(homepath_env, out, p);
        return p;
    }
    return 0;
}

/// Expand a leading `~`, `~/...`, or `~\...` in `path` using `home_dir`.
/// Leaves other-user forms like `~bob/foo` and non-tilde paths untouched.
pub fn expandTilde(path: string, home_dir: string, out: []u8): usize {
    if (path.len == 0 || out.len == 0) {
        return 0;
    }
    const is_tilde_self: bool = path[0] == 126 && (path.len == 1 || isSlash(path[1]));
    if (!is_tilde_self || home_dir.len == 0) {
        return writeStr(path, out, 0);
    }
    if (path.len == 1) {
        return writeStr(home_dir, out, 0);
    }
    const h_len: usize = trimTrailingSlashes(home_dir);
    let p: usize = writeStr(home_dir[0..h_len], out, 0);
    p = writeStr(path[1..path.len], out, p);
    return p;
}

/// Contract a leading `home_dir` prefix in `path` back to `~`.
/// Guards against partial segment false matches (e.g. `/home/alice2` is NOT contracted for `/home/alice`).
pub fn contractTilde(path: string, home_dir: string, out: []u8): usize {
    if (path.len == 0 || out.len == 0) {
        return 0;
    }
    if (home_dir.len == 0) {
        return writeStr(path, out, 0);
    }
    const h_len: usize = trimTrailingSlashes(home_dir);
    const home_clean: string = home_dir[0..h_len];
    if (startsWithBytes(path, home_clean)) {
        if (path.len == h_len) {
            return writeByte(out, 0, 126); // '~'
        }
        if (isSlash(path[h_len])) {
            let p: usize = writeByte(out, 0, 126); // '~'
            p = writeStr(path[h_len..path.len], out, p);
            return p;
        }
    }
    return writeStr(path, out, 0);
}

/// Build an application config directory path following XDG Base Directory semantics:
/// - If `xdg_config_home` is non-empty: `<xdg_config_home>/<app_name>`
/// - Else on POSIX: `<home_dir>/.config/<app_name>`
/// - Else on Windows: `<home_dir>\AppData\Roaming\<app_name>`
/// Returns `0` if both `xdg_config_home` and `home_dir` are empty.
pub fn joinConfigDir(
    home_dir: string,
    xdg_config_home: string,
    app_name: string,
    is_windows: bool,
    out: []u8,
): usize {
    if (out.len == 0) {
        return 0;
    }
    const sep: u8 = if (is_windows) 92 else 47; // '\\' or '/'
    let p: usize = 0;

    if (xdg_config_home.len > 0) {
        const x_len: usize = trimTrailingSlashes(xdg_config_home);
        p = writeStr(xdg_config_home[0..x_len], out, p);
    } else if (home_dir.len > 0) {
        const h_len: usize = trimTrailingSlashes(home_dir);
        p = writeStr(home_dir[0..h_len], out, p);
        p = writeByte(out, p, sep);
        if (is_windows) {
            p = writeStr("AppData\\Roaming", out, p);
        } else {
            p = writeStr(".config", out, p);
        }
    } else {
        return 0;
    }

    if (app_name.len > 0) {
        let a_start: usize = 0;
        while (a_start < app_name.len && isSlash(app_name[a_start])) {
            a_start += 1;
        }
        if (a_start < app_name.len) {
            p = writeByte(out, p, sep);
            p = writeStr(app_name[a_start..app_name.len], out, p);
        }
    }
    return p;
}
