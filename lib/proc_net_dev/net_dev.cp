// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /proc/net/dev Network Interface Stats Parser)

const MAX_U64: u64 = 18446744073709551615;

fn isHWhitespace(c: u8): bool {
    return c == ' ' || c == '\t';
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

fn saturatingAddU64(a: u64, b: u64): u64 {
    if (b > MAX_U64 - a) {
        return MAX_U64;
    }
    return a + b;
}

fn parseDecU64(s: []const u8): u64? {
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
        const d: u64 = u64(c - '0');
        if (val > (MAX_U64 - d) / 10) {
            return null;
        }
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

pub struct NetDevStats {
    iface: []const u8;
    rx_bytes: u64;
    rx_packets: u64;
    rx_errs: u64;
    rx_drop: u64;
    tx_bytes: u64;
    tx_packets: u64;
    tx_errs: u64;
    tx_drop: u64;

    fn totalBytes(): u64 {
        return saturatingAddU64(_.rx_bytes, _.tx_bytes);
    }

    fn totalErrors(): u64 {
        return saturatingAddU64(_.rx_errs, _.tx_errs);
    }

    fn isLoopback(): bool {
        return bytesEql(_.iface, "lo");
    }
}

pub struct ProcNetDevIter {
    src: []const u8;
    pos: usize;

    fn init(content: []const u8): ProcNetDevIter {
        return ProcNetDevIter {
            src: content,
            pos: 0,
        };
    }

    fn next(*_): NetDevStats? {
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

            // 查找最后一个 ':'（支持 iface:12345 粘连及 eth0:0:12345 别名接口）
            let colon_idx = line_end;
            let found_colon = false;
            while (colon_idx > line_start) {
                colon_idx -= 1;
                if (_.src[colon_idx] == ':') {
                    found_colon = true;
                    break;
                }
            }
            if (!found_colon) {
                continue;
            }

            // 裁剪接口名首尾水平空白
            let name_start = line_start;
            let name_end = colon_idx;
            while (name_start < name_end && isHWhitespace(_.src[name_start])) {
                name_start += 1;
            }
            while (name_end > name_start && isHWhitespace(_.src[name_end - 1])) {
                name_end -= 1;
            }
            if (name_start >= name_end) {
                continue;
            }

            // 解析冒号后的全部 16 个统计字段
            let cols: [16]u64 = [0; 16];
            let count: usize = 0;
            let p = colon_idx + 1;
            let valid = true;

            while (p < line_end && count < 16) {
                while (p < line_end && isHWhitespace(_.src[p])) {
                    p += 1;
                }
                if (p >= line_end) {
                    break;
                }
                const tok_start = p;
                while (p < line_end && !isHWhitespace(_.src[p])) {
                    p += 1;
                }
                if (v := parseDecU64(_.src[tok_start..p])) {
                    cols[count] = v;
                    count += 1;
                } else {
                    valid = false;
                    break;
                }
            }

            if (!valid || count < 16) {
                continue;
            }

            return NetDevStats {
                iface: _.src[name_start..name_end],
                rx_bytes: cols[0],
                rx_packets: cols[1],
                rx_errs: cols[2],
                rx_drop: cols[3],
                tx_bytes: cols[8],
                tx_packets: cols[9],
                tx_errs: cols[10],
                tx_drop: cols[11],
            };
        }
        return null;
    }
}

pub fn findNetDev(content: []const u8, iface: []const u8): NetDevStats? {
    if (iface.len == 0) {
        return null;
    }
    let it = ProcNetDevIter.init(content);
    while (entry := it.next()) {
        if (bytesEql(entry.iface, iface)) {
            return entry;
        }
    }
    return null;
}

pub fn sumNonLoopbackBytes(content: []const u8): u64 {
    let it = ProcNetDevIter.init(content);
    let total: u64 = 0;
    while (entry := it.next()) {
        if (!entry.isLoopback()) {
            total = saturatingAddU64(total, entry.totalBytes());
        }
    }
    return total;
}
