// @cap/env-paths — Cross-platform application directory resolver
// Inspired by npm:env-paths (https://github.com/sindresorhus/env-paths)

pub enum EnvPlatform {
    Linux,
    MacOS,
    Windows,
}

pub struct EnvPathsInput {
    platform: EnvPlatform,
    home: string,
    xdg_data_home: string,
    xdg_config_home: string,
    xdg_cache_home: string,
    xdg_state_home: string,
    tmpdir: string,
    local_app_data: string,
    app_data: string,
    suffix: string,

    static fn new(platform: EnvPlatform, home: string): EnvPathsInput {
        return EnvPathsInput{
            platform: platform,
            home: home,
            xdg_data_home: "",
            xdg_config_home: "",
            xdg_cache_home: "",
            xdg_state_home: "",
            tmpdir: "",
            local_app_data: "",
            app_data: "",
            suffix: "-cp",
        };
    }

    static fn linux(home: string): EnvPathsInput {
        return EnvPathsInput.new(EnvPlatform.Linux, home);
    }

    static fn macos(home: string): EnvPathsInput {
        return EnvPathsInput.new(EnvPlatform.MacOS, home);
    }

    static fn windows(home: string): EnvPathsInput {
        return EnvPathsInput.new(EnvPlatform.Windows, home);
    }
}

fn evpTrimTrailingSep(s: string, sep: u8): string {
    let end: usize = s.len;
    while (end > 1 && s[end - 1] == sep) {
        end -= 1;
    }
    return s[0..end];
}

fn evpAppend(out: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn evpAppendByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn evpAppendAppName(out: []u8, pos: usize, name: string, suffix: string): usize {
    let p: usize = evpAppend(out, pos, name);
    if (suffix.len > 0) {
        p = evpAppend(out, p, suffix);
    }
    return p;
}

fn evpJoinUnix2(out: []u8, base: string, name: string, suffix: string): usize {
    const clean: string = evpTrimTrailingSep(base, 0x2F);
    let p: usize = evpAppend(out, 0, clean);
    if (clean.len > 0 && clean[clean.len - 1] != 0x2F) {
        p = evpAppendByte(out, p, 0x2F);
    }
    return evpAppendAppName(out, p, name, suffix);
}

fn evpJoinUnix3(out: []u8, base: string, mid: string, name: string, suffix: string): usize {
    const clean: string = evpTrimTrailingSep(base, 0x2F);
    let p: usize = evpAppend(out, 0, clean);
    if (clean.len > 0 && clean[clean.len - 1] != 0x2F) {
        p = evpAppendByte(out, p, 0x2F);
    }
    p = evpAppend(out, p, mid);
    p = evpAppendByte(out, p, 0x2F);
    return evpAppendAppName(out, p, name, suffix);
}

fn evpWinAppBase(out: []u8, explicit: string, home: string, fallback_sub: string): usize {
    if (explicit.len > 0) {
        const clean: string = evpTrimTrailingSep(explicit, 0x5C);
        return evpAppend(out, 0, clean);
    }
    const clean_home: string = evpTrimTrailingSep(home, 0x5C);
    let p: usize = evpAppend(out, 0, clean_home);
    if (clean_home.len > 0 && clean_home[clean_home.len - 1] != 0x5C) {
        p = evpAppendByte(out, p, 0x5C);
    }
    return evpAppend(out, p, fallback_sub);
}

fn evpWinDir(
    out: []u8,
    explicit: string,
    home: string,
    fallback_sub: string,
    name: string,
    suffix: string,
    leaf: string
): usize {
    let p: usize = evpWinAppBase(out, explicit, home, fallback_sub);
    p = evpAppendByte(out, p, 0x5C);
    p = evpAppendAppName(out, p, name, suffix);
    if (leaf.len > 0) {
        p = evpAppendByte(out, p, 0x5C);
        p = evpAppend(out, p, leaf);
    }
    return p;
}

/// Resolves the platform-appropriate data directory into `out`.
pub fn envDataDir(name: string, inp: EnvPathsInput, out: []u8): usize {
    if (inp.platform == EnvPlatform.Linux) {
        if (inp.xdg_data_home.len > 0) {
            return evpJoinUnix2(out, inp.xdg_data_home, name, inp.suffix);
        }
        return evpJoinUnix3(out, inp.home, ".local/share", name, inp.suffix);
    }
    if (inp.platform == EnvPlatform.MacOS) {
        return evpJoinUnix3(out, inp.home, "Library/Application Support", name, inp.suffix);
    }
    return evpWinDir(out, inp.local_app_data, inp.home, "AppData\\Local", name, inp.suffix, "Data");
}

/// Resolves the platform-appropriate config directory into `out`.
pub fn envConfigDir(name: string, inp: EnvPathsInput, out: []u8): usize {
    if (inp.platform == EnvPlatform.Linux) {
        if (inp.xdg_config_home.len > 0) {
            return evpJoinUnix2(out, inp.xdg_config_home, name, inp.suffix);
        }
        return evpJoinUnix3(out, inp.home, ".config", name, inp.suffix);
    }
    if (inp.platform == EnvPlatform.MacOS) {
        return evpJoinUnix3(out, inp.home, "Library/Preferences", name, inp.suffix);
    }
    return evpWinDir(out, inp.app_data, inp.home, "AppData\\Roaming", name, inp.suffix, "Config");
}

/// Resolves the platform-appropriate cache directory into `out`.
pub fn envCacheDir(name: string, inp: EnvPathsInput, out: []u8): usize {
    if (inp.platform == EnvPlatform.Linux) {
        if (inp.xdg_cache_home.len > 0) {
            return evpJoinUnix2(out, inp.xdg_cache_home, name, inp.suffix);
        }
        return evpJoinUnix3(out, inp.home, ".cache", name, inp.suffix);
    }
    if (inp.platform == EnvPlatform.MacOS) {
        return evpJoinUnix3(out, inp.home, "Library/Caches", name, inp.suffix);
    }
    return evpWinDir(out, inp.local_app_data, inp.home, "AppData\\Local", name, inp.suffix, "Cache");
}

/// Resolves the platform-appropriate log directory into `out`.
pub fn envLogDir(name: string, inp: EnvPathsInput, out: []u8): usize {
    if (inp.platform == EnvPlatform.Linux) {
        if (inp.xdg_state_home.len > 0) {
            return evpJoinUnix2(out, inp.xdg_state_home, name, inp.suffix);
        }
        return evpJoinUnix3(out, inp.home, ".local/state", name, inp.suffix);
    }
    if (inp.platform == EnvPlatform.MacOS) {
        return evpJoinUnix3(out, inp.home, "Library/Logs", name, inp.suffix);
    }
    return evpWinDir(out, inp.local_app_data, inp.home, "AppData\\Local", name, inp.suffix, "Log");
}

/// Resolves the platform-appropriate temporary directory into `out`.
pub fn envTempDir(name: string, inp: EnvPathsInput, out: []u8): usize {
    if (inp.platform == EnvPlatform.Windows) {
        if (inp.tmpdir.len > 0) {
            return evpWinDir(out, inp.tmpdir, inp.home, "", name, inp.suffix, "");
        }
        return evpWinDir(out, "", inp.home, "AppData\\Local\\Temp", name, inp.suffix, "");
    }
    const base: string = if (inp.tmpdir.len > 0) inp.tmpdir else "/tmp";
    return evpJoinUnix2(out, base, name, inp.suffix);
}
