// Source: Absorbed from /data/dev/bun/src/sys.zig (Linux /etc/protocols Parser)
// Pure-CP zero-allocation Linux /etc/protocols streaming parser and bidirectional lookup utilities.

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
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

fn parseU16Strict(s: []const u8): u16? {
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
    return @as(u16, @intCast(val));
}

/// Represents a single parsed `/etc/protocols` entry: `protocol-name number [aliases...] [# comment]`.
pub struct ProtocolEntry {
    name: []const u8,
    number: u16,
    aliases: []const u8,

    /// Parse a single `/etc/protocols` line, stripping trailing `\r` and `#` inline comments.
    fn parseLine(raw: []const u8): ProtocolEntry? {
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

        // 1. Parse protocol name
        let pos: usize = 0;
        while (pos < line.len && !isWs(line[pos])) {
            pos += 1;
        }
        const name = line[0..pos];
        if (name.len == 0) return null;

        // Skip whitespace before protocol number
        while (pos < line.len && isWs(line[pos])) {
            pos += 1;
        }
        if (pos >= line.len) return null;

        // 2. Parse protocol number (0..=65535)
        const num_start = pos;
        while (pos < line.len && !isWs(line[pos])) {
            pos += 1;
        }
        const num_tok = line[num_start..pos];
        const number = parseU16Strict(num_tok) ?? return null;

        // 3. Remaining trimmed slice is whitespace-separated aliases
        while (pos < line.len && isWs(line[pos])) {
            pos += 1;
        }
        const aliases = line[pos..line.len];

        return ProtocolEntry {
            name: name,
            number: number,
            aliases: aliases,
        };
    }

    /// Case-insensitive exact match against primary protocol name or any whitespace-separated alias.
    fn hasNameOrAlias(query: []const u8): bool {
        if (query.len == 0) return false;
        if (strEqIgnoreCase(_.name, query)) return true;
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
            if (pos > start && strEqIgnoreCase(_.aliases[start..pos], query)) {
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

/// Zero-allocation streaming iterator over `/etc/protocols` file contents.
pub struct EtcProtocolsIter {
    src: []const u8,
    pos: usize,

    fn init(content: []const u8): EtcProtocolsIter {
        return EtcProtocolsIter {
            src: content,
            pos: 0,
        };
    }

    fn next(*_): ProtocolEntry? {
        while (_.pos < _.src.len) {
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                _.pos += 1;
            }
            const end = _.pos;
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            if (entry := ProtocolEntry.parseLine(_.src[start..end])) {
                return entry;
            }
        }
        return null;
    }
}

/// Lookup protocol number by primary name or alias (ASCII case-insensitive).
pub fn findProtocolNumber(content: []const u8, name: []const u8): u16? {
    if (name.len == 0) return null;
    let it = EtcProtocolsIter.init(content);
    while (entry := it.next()) {
        if (entry.hasNameOrAlias(name)) {
            return entry.number;
        }
    }
    return null;
}

/// Reverse lookup protocol entry by protocol number.
pub fn findProtocolByNumber(content: []const u8, number: u16): ProtocolEntry? {
    let it = EtcProtocolsIter.init(content);
    while (entry := it.next()) {
        if (entry.number == number) {
            return entry;
        }
    }
    return null;
}
