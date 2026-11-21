// Source: Absorbed from /data/dev/bun/src/cli.zig (Vixie Cron Macro Presets & 5-Field Normalizer)
// Pure-CP zero-allocation Vixie/systemd cron preset expander (@hourly/@daily/@weekly/@monthly/@yearly/@reboot).

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

export enum CronPreset {
    yearly,
    monthly,
    weekly,
    daily,
    hourly,
    reboot,
}

/// Parse a Vixie cron `@macro` name (case-insensitive; `@annually` -> `yearly`, `@midnight` -> `daily`).
export fn parseCronPreset(raw: []const u8): CronPreset? {
    const s = trimWs(raw);
    if (eqIgnoreCase(s, "@yearly") || eqIgnoreCase(s, "@annually")) return CronPreset.yearly;
    if (eqIgnoreCase(s, "@monthly")) return CronPreset.monthly;
    if (eqIgnoreCase(s, "@weekly")) return CronPreset.weekly;
    if (eqIgnoreCase(s, "@daily") || eqIgnoreCase(s, "@midnight")) return CronPreset.daily;
    if (eqIgnoreCase(s, "@hourly")) return CronPreset.hourly;
    if (eqIgnoreCase(s, "@reboot")) return CronPreset.reboot;
    return null;
}

/// Expand a `CronPreset` to its canonical 5-field cron schedule (`null` for `@reboot`).
export fn presetToCron5(preset: CronPreset): ([]const u8)? {
    if (preset == CronPreset.yearly) return "0 0 1 1 *";
    if (preset == CronPreset.monthly) return "0 0 1 * *";
    if (preset == CronPreset.weekly) return "0 0 * * 0";
    if (preset == CronPreset.daily) return "0 0 * * *";
    if (preset == CronPreset.hourly) return "0 * * * *";
    return null;
}

/// Expand a `@macro` string to a 5-field cron expression, or return `raw` trimmed if it already has 5 whitespace-separated fields.
/// Returns `null` for `@reboot`, unknown `@macros`, or expressions with != 5 fields.
export fn normalizeCron5(raw: []const u8): ([]const u8)? {
    const s = trimWs(raw);
    if (s.len == 0) return null;
    if (s[0] == '@') {
        const p = parseCronPreset(s) ?? return null;
        return presetToCron5(p);
    }
    let fields: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        while (i < s.len && isWs(s[i])) i += 1;
        if (i >= s.len) break;
        fields += 1;
        while (i < s.len && !isWs(s[i])) i += 1;
    }
    if (fields != 5) return null;
    return s;
}
