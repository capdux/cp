// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T16:05:03Z (Linux /proc/net/route Routing Table Parser)

pub const RTF_UP: u32 = 0x0001;
pub const RTF_GATEWAY: u32 = 0x0002;
pub const RTF_HOST: u32 = 0x0004;

fn isHWhitespace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn hexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn parseHexByte(hi_c: u8, lo_c: u8): u8? {
    const hi = hexNibble(hi_c) ?? return null;
    const lo = hexNibble(lo_c) ?? return null;
    return (hi << 4) | lo;
}

fn parseHexU32(s: []const u8): u32? {
    if (s.len == 0 || s.len > 8) {
        return null;
    }
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const d = hexNibble(s[i]) ?? return null;
        val = (val << 4) | u32(d);
        i += 1;
    }
    return val;
}

fn parseDecU32(s: []const u8): u32? {
    if (s.len == 0) {
        return null;
    }
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        val = val * 10 + u64(c - '0');
        if (val > 0xFFFFFFFF) {
            return null;
        }
        i += 1;
    }
    return val as u32;
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn parseHexIpv4Le(hex8: []const u8): ([4]u8)? {
    if (hex8.len != 8) {
        return null;
    }
    const b0 = parseHexByte(hex8[0], hex8[1]) ?? return null;
    const b1 = parseHexByte(hex8[2], hex8[3]) ?? return null;
    const b2 = parseHexByte(hex8[4], hex8[5]) ?? return null;
    const b3 = parseHexByte(hex8[6], hex8[7]) ?? return null;
    return [b3, b2, b1, b0];
}

pub struct RouteEntry {
    iface: []const u8;
    destination: [4]u8;
    gateway: [4]u8;
    flags: u32;
    metric: u32;
    mask: [4]u8;
    mtu: u32;

    fn isUp(): bool {
        return (_.flags & RTF_UP) != 0;
    }

    fn isGateway(): bool {
        return (_.flags & RTF_GATEWAY) != 0;
    }

    fn isDefaultRoute(): bool {
        if (!_.isUp()) {
            return false;
        }
        return _.destination[0] == 0
            && _.destination[1] == 0
            && _.destination[2] == 0
            && _.destination[3] == 0
            && _.mask[0] == 0
            && _.mask[1] == 0
            && _.mask[2] == 0
            && _.mask[3] == 0;
    }

    fn prefixLen(): u8 {
        let count: u8 = 0;
        let i: usize = 0;
        while (i < 4) {
            const oct = _.mask[i];
            let bit: u8 = 0;
            while (bit < 8) {
                const shift: u3 = (7 - bit) as u3;
                if (((oct >> shift) & 1) == 1) {
                    count += 1;
                } else {
                    return count;
                }
                bit += 1;
            }
            i += 1;
        }
        return count;
    }
}

pub struct ProcRouteIter {
    src: []const u8;
    pos: usize;

    fn init(content: []const u8): ProcRouteIter {
        return ProcRouteIter {
            src: content,
            pos: 0,
        };
    }

    fn next(*_): RouteEntry? {
        while (_.pos < _.src.len) {
            const line_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
                _.pos += 1;
            }
            const line_end = _.pos;
            if (_.pos < _.src.len) {
                if (_.src[_.pos] == '\r') {
                    _.pos += 1;
                    if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                        _.pos += 1;
                    }
                } else if (_.src[_.pos] == '\n') {
                    _.pos += 1;
                }
            }

            // 切分标准 11 列（零分配记录各列起止偏移）
            let col_start: [11]usize = [0; 11];
            let col_end: [11]usize = [0; 11];
            let col_count: usize = 0;
            let p = line_start;

            while (p < line_end && col_count < 11) {
                while (p < line_end && isHWhitespace(_.src[p])) {
                    p += 1;
                }
                if (p >= line_end) {
                    break;
                }
                col_start[col_count] = p;
                while (p < line_end && !isHWhitespace(_.src[p])) {
                    p += 1;
                }
                col_end[col_count] = p;
                col_count += 1;
            }

            if (col_count < 11) {
                continue;
            }

            const iface = _.src[col_start[0]..col_end[0]];
            if (bytesEql(iface, "Iface")) {
                continue;
            }

            const dest = parseHexIpv4Le(_.src[col_start[1]..col_end[1]]) ?? continue;
            const gw = parseHexIpv4Le(_.src[col_start[2]..col_end[2]]) ?? continue;
            const flags = parseHexU32(_.src[col_start[3]..col_end[3]]) ?? continue;
            // 校验 RefCnt (col 4), Use (col 5)
            * = parseDecU32(_.src[col_start[4]..col_end[4]]) ?? continue;
            * = parseDecU32(_.src[col_start[5]..col_end[5]]) ?? continue;
            const metric = parseDecU32(_.src[col_start[6]..col_end[6]]) ?? continue;
            const mask = parseHexIpv4Le(_.src[col_start[7]..col_end[7]]) ?? continue;
            const mtu = parseDecU32(_.src[col_start[8]..col_end[8]]) ?? continue;
            // 校验 Window (col 9), IRTT (col 10)
            * = parseDecU32(_.src[col_start[9]..col_end[9]]) ?? continue;
            * = parseDecU32(_.src[col_start[10]..col_end[10]]) ?? continue;

            return RouteEntry {
                iface: iface,
                destination: dest,
                gateway: gw,
                flags: flags,
                metric: metric,
                mask: mask,
                mtu: mtu,
            };
        }
        return null;
    }
}

pub fn findDefaultGateway(content: []const u8): RouteEntry? {
    let it = ProcRouteIter.init(content);
    let best: RouteEntry? = null;

    while (entry := it.next()) {
        if (!entry.isDefaultRoute() || !entry.isGateway()) {
            continue;
        }
        if (cur := best) {
            if (entry.metric < cur.metric) {
                best = entry;
            }
        } else {
            best = entry;
        }
    }

    return best;
}
