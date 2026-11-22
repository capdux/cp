// Source: Absorbed from /data/dev/bun/src/string/immutable.zig | Synced: 2026-11-22T07:17:50Z (Identifier Case Converter)
// Pure-CP zero-allocation acronym-aware identifier case converter (snake, screaming_snake, kebab, camel, pascal).

fn isUpper(b: u8): bool {
    return b >= 'A' && b <= 'Z';
}

fn isLower(b: u8): bool {
    return b >= 'a' && b <= 'z';
}

fn isDigit(b: u8): bool {
    return b >= '0' && b <= '9';
}

fn isAlnum(b: u8): bool {
    return isUpper(b) || isLower(b) || isDigit(b);
}

fn toLower(b: u8): u8 {
    if (isUpper(b)) return b + 32;
    return b;
}

fn toUpper(b: u8): u8 {
    if (isLower(b)) return b - 32;
    return b;
}

/// Zero-copy word splitter for identifiers (`HTTPServer` -> `HTTP`, `Server`; `foo_bar-baz` -> `foo`, `bar`, `baz`).
export struct WordIter {
    src: []const u8,
    pos: usize,

    pub fn init(src: []const u8): WordIter {
        return WordIter { src: src, pos: 0 };
    }

    pub fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len && !isAlnum(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= _.src.len) return null;

        const start = _.pos;
        _.pos += 1;
        while (_.pos < _.src.len && isAlnum(_.src[_.pos])) {
            const prev = _.src[_.pos - 1];
            const cur = _.src[_.pos];
            // Split lower/digit -> Upper (e.g. "fooBar", "v2Api")
            if ((isLower(prev) || isDigit(prev)) && isUpper(cur)) {
                break;
            }
            // Split acronym before Title word: "HTTPServer" -> "HTTP" + "Server"
            if (isUpper(prev) && isUpper(cur) && _.pos + 1 < _.src.len && isLower(_.src[_.pos + 1])) {
                break;
            }
            _.pos += 1;
        }
        return _.src[start.._.pos];
    }
}

fn convertDelimited(src: []const u8, delim: u8, upper: bool, out: []u8): ([]const u8)? {
    let it = WordIter.init(src);
    let pos: usize = 0;
    let first = true;
    while (word := it.next()) {
        if (!first) {
            if (pos >= out.len) return null;
            out[pos] = delim;
            pos += 1;
        }
        first = false;
        if (pos + word.len > out.len) return null;
        let i: usize = 0;
        while (i < word.len) {
            out[pos + i] = if (upper) toUpper(word[i]) else toLower(word[i]);
            i += 1;
        }
        pos += word.len;
    }
    return out[0..pos];
}

export fn toSnakeCase(src: []const u8, out: []u8): ([]const u8)? {
    return convertDelimited(src, '_', false, out);
}

export fn toScreamingSnakeCase(src: []const u8, out: []u8): ([]const u8)? {
    return convertDelimited(src, '_', true, out);
}

export fn toKebabCase(src: []const u8, out: []u8): ([]const u8)? {
    return convertDelimited(src, '-', false, out);
}

export fn toCamelCase(src: []const u8, out: []u8): ([]const u8)? {
    let it = WordIter.init(src);
    let pos: usize = 0;
    let first = true;
    while (word := it.next()) {
        if (pos + word.len > out.len) return null;
        let i: usize = 0;
        while (i < word.len) {
            if (first) {
                out[pos + i] = toLower(word[i]);
            } else if (i == 0) {
                out[pos + i] = toUpper(word[i]);
            } else {
                out[pos + i] = toLower(word[i]);
            }
            i += 1;
        }
        pos += word.len;
        first = false;
    }
    return out[0..pos];
}

export fn toPascalCase(src: []const u8, out: []u8): ([]const u8)? {
    let it = WordIter.init(src);
    let pos: usize = 0;
    while (word := it.next()) {
        if (pos + word.len > out.len) return null;
        let i: usize = 0;
        while (i < word.len) {
            out[pos + i] = if (i == 0) toUpper(word[i]) else toLower(word[i]);
            i += 1;
        }
        pos += word.len;
    }
    return out[0..pos];
}
