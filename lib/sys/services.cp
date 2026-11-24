// Source: Absorbed from /data/dev/bun/src/sys.zig | Synced: 2026-11-21T16:24:22Z (Linux /etc/services Parser)
// Pure-CP zero-allocation Linux /etc/services streaming parser and bidirectional lookup utilities.

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn strEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isWs(s[start])) {
        start += 1;
    }
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn parsePortStrict(s: []const u8): u16? {
    if (s.len == 0) return null;
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + @as(u32, c - '0');
        if (val > 65535) return null;
        i += 1;
    }
    if (val == 0) return null;
    return @as(u16, @intCast(val));
}

/// Represents a single parsed `/etc/services` entry: `service-name port/protocol [aliases...]`.
pub struct ServiceEntry {
    name: []const u8,
    port: u16,
    proto: []const u8,
    aliases: []const u8,

    /// Parse a single `/etc/services` line, stripping trailing `\r` and `#` comments.
    static fn parseLine(raw: []const u8): ServiceEntry? {
        let end = raw.len;
        if (end > 0 && raw[end - 1] == '\r') {
            end -= 1;
        }

        // Strip inline `#` comment
        let hash_idx: usize = 0;
        while (hash_idx < end && raw[hash_idx] != '#') {
            hash_idx += 1;
        }

        const line = trimWs(raw[0..hash_idx]);
        if (line.len == 0) return null;

        // 1. Parse service name
        let pos: usize = 0;
        while (pos < line.len && !isWs(line[pos])) {
            pos += 1;
        }
        const name = line[0..pos];
        if (name.len == 0) return null;

        // Skip whitespace before port/proto
        while (pos < line.len && isWs(line[pos])) {
            pos += 1;
        }
        if (pos >= line.len) return null;

        // 2. Parse port/proto token
        const pp_start = pos;
        while (pos < line.len && !isWs(line[pos])) {
            pos += 1;
        }
        const pp_tok = line[pp_start..pos];

        // Locate single `/` separating port and protocol
        let slash_idx: usize = 0;
        while (slash_idx < pp_tok.len && pp_tok[slash_idx] != '/') {
            slash_idx += 1;
        }
        if (slash_idx == 0 || slash_idx + 1 >= pp_tok.len) return null;

        const port_str = pp_tok[0..slash_idx];
        const proto = pp_tok[slash_idx + 1..pp_tok.len];

        // Reject extra `/` inside protocol
        let k: usize = 0;
        while (k < proto.len) {
            if (proto[k] == '/') return null;
            k += 1;
        }

        const port = parsePortStrict(port_str) ?? return null;

        // 3. Remaining trimmed slice is the whitespace-delimited alias list
        while (pos < line.len && isWs(line[pos])) {
            pos += 1;
        }
        const aliases = line[pos..line.len];

        return ServiceEntry {
            name: name,
            port: port,
            proto: proto,
            aliases: aliases,
        };
    }

    /// Returns true if `query` exactly matches the primary service name or any whitespace-separated alias.
    fn hasNameOrAlias(query: []const u8): bool {
        if (query.len == 0) return false;
        if (strEq(_.name, query)) return true;
        if (_.aliases.len == 0) return false;

        let pos: usize = 0;
        while (pos < _.aliases.len) {
            while (pos < _.aliases.len && isWs(_.aliases[pos])) {
                pos += 1;
            }
            const start = pos;
            while (pos < _.aliases.len && !isWs(_.aliases[pos])) {
                pos += 1;
            }
            if (pos > start && strEq(_.aliases[start..pos], query)) {
                return true;
            }
        }
        return false;
    }

    /// Counts whitespace-separated aliases in `aliases`.
    fn aliasCount(): usize {
        let count: usize = 0;
        let pos: usize = 0;
        while (pos < _.aliases.len) {
            while (pos < _.aliases.len && isWs(_.aliases[pos])) {
                pos += 1;
            }
            const start = pos;
            while (pos < _.aliases.len && !isWs(_.aliases[pos])) {
                pos += 1;
            }
            if (pos > start) {
                count += 1;
            }
        }
        return count;
    }
}

/// Zero-allocation streaming iterator over `/etc/services` file contents.
pub struct EtcServicesIter {
    src: []const u8,
    pos: usize,

    static fn init(content: []const u8): EtcServicesIter {
        return EtcServicesIter {
            src: content,
            pos: 0,
        };
    }

    fn next(*_): ServiceEntry? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            if (entry := ServiceEntry.parseLine(_.src[start..end])) {
                return entry;
            }
        }
        return null;
    }
}

/// Lookup port number by primary service name or alias and case-insensitive protocol (e.g. `"tcp"`, `"UDP"`).
pub fn findServicePort(content: []const u8, name: []const u8, proto: []const u8): u16? {
    if (name.len == 0 || proto.len == 0) return null;
    let it = EtcServicesIter.init(content);
    while (entry := it.next()) {
        if (strEqIgnoreCase(entry.proto, proto) && entry.hasNameOrAlias(name)) {
            return entry.port;
        }
    }
    return null;
}

/// Reverse lookup service entry by port number and case-insensitive protocol.
pub fn findServiceByPort(content: []const u8, port: u16, proto: []const u8): ServiceEntry? {
    if (port == 0 || proto.len == 0) return null;
    let it = EtcServicesIter.init(content);
    while (entry := it.next()) {
        if (entry.port == port && strEqIgnoreCase(entry.proto, proto)) {
            return entry;
        }
    }
    return null;
}
