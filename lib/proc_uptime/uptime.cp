// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/uptime Zero-Alloc Integer Parser)

const MAX_U64: u64 = 18446744073709551615;

fn isWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn parseSecondsToMs(tok: []const u8): u64? {
    if (tok.len == 0) {
        return null;
    }
    let i: usize = 0;
    let int_part: u64 = 0;
    let saw_int = false;

    while (i < tok.len && tok[i] >= '0' && tok[i] <= '9') {
        saw_int = true;
        const d: u64 = u64(tok[i] - '0');
        if (int_part > (MAX_U64 - d) / 10) {
            return null;
        }
        int_part = int_part * 10 + d;
        i += 1;
    }
    if (!saw_int) {
        return null;
    }
    if (int_part > MAX_U64 / 1000) {
        return null;
    }

    let frac_part: u64 = 0;
    let frac_digits: usize = 0;
    if (i < tok.len) {
        if (tok[i] != '.') {
            return null;
        }
        i += 1;
        if (i >= tok.len) {
            return null;
        }
        while (i < tok.len) {
            const c = tok[i];
            if (c < '0' || c > '9') {
                return null;
            }
            if (frac_digits < 3) {
                frac_part = frac_part * 10 + u64(c - '0');
                frac_digits += 1;
            }
            i += 1;
        }
    }

    while (frac_digits < 3) {
        frac_part *= 10;
        frac_digits += 1;
    }

    const base_ms = int_part * 1000;
    if (frac_part > MAX_U64 - base_ms) {
        return null;
    }
    return base_ms + frac_part;
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) {
        return false;
    }
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeDecU64(out: []u8, pos: *usize, value: u64): bool {
    let tmp: [20]u8 = [0; 20];
    let n: usize = 0;
    let v = value;
    if (v == 0) {
        return writeByte(out, pos, '0');
    }
    while (v > 0) {
        tmp[n] = ('0' + ((v % 10) as u8)) as u8;
        v /= 10;
        n += 1;
    }
    while (n > 0) {
        n -= 1;
        if (!writeByte(out, pos, tmp[n])) {
            return false;
        }
    }
    return true;
}

fn writeTwoDigits(out: []u8, pos: *usize, value: u64): bool {
    const hi: u8 = ('0' + (((value / 10) % 10) as u8)) as u8;
    const lo: u8 = ('0' + ((value % 10) as u8)) as u8;
    if (!writeByte(out, pos, hi)) return false;
    return writeByte(out, pos, lo);
}

pub struct ProcUptime {
    uptime_ms: u64;
    idle_ms: u64;

    fn uptimeSeconds(): u64 {
        return _.uptime_ms / 1000;
    }

    fn idleSeconds(): u64 {
        return _.idle_ms / 1000;
    }

    fn idleRatioPermille(cpu_count: u32): u32 {
        if (cpu_count == 0 || _.uptime_ms == 0) {
            return 0;
        }
        const denom: u128 = u128(_.uptime_ms) * u128(cpu_count);
        const ratio: u128 = (u128(_.idle_ms) * 1000) / denom;
        if (ratio >= 1000) {
            return 1000;
        }
        return ratio as u32;
    }

    fn formatHuman(out: []u8): ([]const u8)? {
        const total_sec = _.uptime_ms / 1000;
        const days = total_sec / 86400;
        const hours = (total_sec % 86400) / 3600;
        const mins = (total_sec % 3600) / 60;
        const secs = total_sec % 60;

        let pos: usize = 0;
        if (days > 0) {
            if (!writeDecU64(out, *pos, days)) return null;
            if (!writeByte(out, *pos, 'd')) return null;
            if (!writeByte(out, *pos, ' ')) return null;
            if (!writeTwoDigits(out, *pos, hours)) return null;
            if (!writeByte(out, *pos, 'h')) return null;
            if (!writeByte(out, *pos, ' ')) return null;
            if (!writeTwoDigits(out, *pos, mins)) return null;
            if (!writeByte(out, *pos, 'm')) return null;
            if (!writeByte(out, *pos, ' ')) return null;
            if (!writeTwoDigits(out, *pos, secs)) return null;
            if (!writeByte(out, *pos, 's')) return null;
        } else if (hours > 0) {
            if (!writeDecU64(out, *pos, hours)) return null;
            if (!writeByte(out, *pos, 'h')) return null;
            if (!writeByte(out, *pos, ' ')) return null;
            if (!writeTwoDigits(out, *pos, mins)) return null;
            if (!writeByte(out, *pos, 'm')) return null;
            if (!writeByte(out, *pos, ' ')) return null;
            if (!writeTwoDigits(out, *pos, secs)) return null;
            if (!writeByte(out, *pos, 's')) return null;
        } else if (mins > 0) {
            if (!writeDecU64(out, *pos, mins)) return null;
            if (!writeByte(out, *pos, 'm')) return null;
            if (!writeByte(out, *pos, ' ')) return null;
            if (!writeTwoDigits(out, *pos, secs)) return null;
            if (!writeByte(out, *pos, 's')) return null;
        } else {
            if (!writeDecU64(out, *pos, secs)) return null;
            if (!writeByte(out, *pos, 's')) return null;
        }

        return out[0..pos];
    }
}

pub fn parseProcUptime(content: []const u8): ProcUptime? {
    let i: usize = 0;

    while (i < content.len && isWhitespace(content[i])) {
        i += 1;
    }
    const t1_start = i;
    while (i < content.len && !isWhitespace(content[i])) {
        i += 1;
    }
    const t1 = content[t1_start..i];
    if (t1.len == 0) {
        return null;
    }

    while (i < content.len && isWhitespace(content[i])) {
        i += 1;
    }
    const t2_start = i;
    while (i < content.len && !isWhitespace(content[i])) {
        i += 1;
    }
    const t2 = content[t2_start..i];
    if (t2.len == 0) {
        return null;
    }

    while (i < content.len && isWhitespace(content[i])) {
        i += 1;
    }
    if (i < content.len) {
        // 拒绝多余的第 3 个字段
        return null;
    }

    const uptime_ms = parseSecondsToMs(t1) ?? return null;
    const idle_ms = parseSecondsToMs(t2) ?? return null;

    return ProcUptime {
        uptime_ms: uptime_ms,
        idle_ms: idle_ms,
    };
}
