// Source: /data/dev/bun/src/string/
// lib/string/case.cp
// Zero-heap Identifier Naming Case Converter (snake_case, kebab-case, camelCase, PascalCase, SCREAMING_SNAKE_CASE)
// Absorbed from Bun src/string/

pub struct CaseConverter {
    pub static fn isUpper(c: u8): bool {
        return c >= 'A' && c <= 'Z';
    }

    pub static fn isLower(c: u8): bool {
        return c >= 'a' && c <= 'z';
    }

    pub static fn isDigit(c: u8): bool {
        return c >= '0' && c <= '9';
    }

    pub static fn isAlnum(c: u8): bool {
        return CaseConverter.isUpper(c) || CaseConverter.isLower(c) || CaseConverter.isDigit(c);
    }

    pub static fn toLowerByte(c: u8): u8 {
        return if (CaseConverter.isUpper(c)) (c + 32) else c;
    }

    pub static fn toUpperByte(c: u8): u8 {
        return if (CaseConverter.isLower(c)) (c - 32) else c;
    }

    /// Splits an identifier (`camelCase`, `PascalCase`, `snake_case`, `kebab-case`, `HTTPRequest`) into words.
    pub static fn splitWords(input: []const u8, out_words: [][]const u8): usize {
        let count: usize = 0;
        let i: usize = 0;

        while (i < input.len && count < out_words.len) {
            while (i < input.len && !CaseConverter.isAlnum(input[i])) {
                i += 1;
            }
            if (i >= input.len) {
                break;
            }
            const start = i;
            i += 1;
            while (i < input.len && CaseConverter.isAlnum(input[i])) {
                const prev = input[i - 1];
                const curr = input[i];
                // Boundary: lowercase/digit followed by uppercase (`userId` -> `user` | `Id`)
                if ((CaseConverter.isLower(prev) || CaseConverter.isDigit(prev)) && CaseConverter.isUpper(curr)) {
                    break;
                }
                // Boundary: acronym followed by Upper+lower (`HTMLParser` -> `HTML` | `Parser`)
                if (CaseConverter.isUpper(prev) && CaseConverter.isUpper(curr) &&
                    (i + 1 < input.len) && CaseConverter.isLower(input[i + 1]))
                {
                    break;
                }
                i += 1;
            }
            out_words[count] = input[start..i];
            count += 1;
        }
        return count;
    }

    pub static fn formatDelimited(input: []const u8, sep: u8, uppercase: bool, out: []u8): usize {
        let words: [32][]const u8 = undefined;
        const count = CaseConverter.splitWords(input, words[0..32]);
        let pos: usize = 0;
        let w: usize = 0;

        while (w < count) {
            if (w > 0) {
                if (pos >= out.len) {
                    break;
                }
                out[pos] = sep;
                pos += 1;
            }
            const word = words[w];
            let k: usize = 0;
            while (k < word.len && pos < out.len) {
                out[pos] = if (uppercase) CaseConverter.toUpperByte(word[k]) else CaseConverter.toLowerByte(word[k]);
                pos += 1;
                k += 1;
            }
            w += 1;
        }
        return pos;
    }

    pub static fn toSnakeCase(input: []const u8, out: []u8): usize {
        return CaseConverter.formatDelimited(input, '_', false, out);
    }

    pub static fn toKebabCase(input: []const u8, out: []u8): usize {
        return CaseConverter.formatDelimited(input, '-', false, out);
    }

    pub static fn toScreamingSnakeCase(input: []const u8, out: []u8): usize {
        return CaseConverter.formatDelimited(input, '_', true, out);
    }

    pub static fn toCamelOrPascal(input: []const u8, pascal: bool, out: []u8): usize {
        let words: [32][]const u8 = undefined;
        const count = CaseConverter.splitWords(input, words[0..32]);
        let pos: usize = 0;
        let w: usize = 0;

        while (w < count) {
            const word = words[w];
            let k: usize = 0;
            while (k < word.len && pos < out.len) {
                if (k == 0 && (pascal || w > 0)) {
                    out[pos] = CaseConverter.toUpperByte(word[k]);
                } else {
                    out[pos] = CaseConverter.toLowerByte(word[k]);
                }
                pos += 1;
                k += 1;
            }
            w += 1;
        }
        return pos;
    }

    pub static fn toCamelCase(input: []const u8, out: []u8): usize {
        return CaseConverter.toCamelOrPascal(input, false, out);
    }

    pub static fn toPascalCase(input: []const u8, out: []u8): usize {
        return CaseConverter.toCamelOrPascal(input, true, out);
    }
}
