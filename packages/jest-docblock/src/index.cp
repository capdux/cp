// @cap/jest-docblock — Zero-heap docblock pragma extractor, parser, and printer in pure CP.

pub struct DocPragma {
    key: string,
    value: string,
}

fn jdblIsWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn jdblIsLineWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r';
}

fn jdblTrimLine(s: string): string {
    let start: usize = 0;
    while (start < s.len && jdblIsLineWs(s[start])) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && jdblIsLineWs(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn jdblWriteStr(src: string, out: []u8, pos: usize): usize {
    let w = pos;
    let i: usize = 0;
    while (i < src.len && w < out.len) : (i += 1) {
        out[w] = src[i];
        w += 1;
    }
    return w;
}

pub fn extractDocblock(code: string): string {
    let i: usize = 0;
    while (i < code.len && jdblIsWs(code[i])) : (i += 1) {}

    // Skip optional shebang line
    if (i + 1 < code.len && code[i] == '#' && code[i + 1] == '!') {
        while (i < code.len && code[i] != '\n') : (i += 1) {}
        while (i < code.len && jdblIsWs(code[i])) : (i += 1) {}
    }

    if (i + 2 >= code.len || code[i] != '/' || code[i + 1] != '*' || code[i + 2] != '*') {
        return "";
    }

    let j = i + 3;
    while (j + 1 < code.len) : (j += 1) {
        if (code[j] == '*' && code[j + 1] == '/') {
            return code[i..j + 2];
        }
    }
    return "";
}

pub fn parseDocblock(docblock: string, out: []DocPragma): usize {
    let body = docblock;
    if (body.len >= 5 && body[0] == '/' && body[1] == '*' && body[2] == '*' && body[body.len - 2] == '*' && body[body.len - 1] == '/') {
        body = body[3..body.len - 2];
    } else if (body.len >= 4 && body[0] == '/' && body[1] == '*' && body[body.len - 2] == '*' && body[body.len - 1] == '/') {
        body = body[2..body.len - 2];
    }

    let written: usize = 0;
    let pos: usize = 0;
    while (pos <= body.len) {
        let lineEnd = pos;
        while (lineEnd < body.len && body[lineEnd] != '\n') : (lineEnd += 1) {}

        let line = jdblTrimLine(body[pos..lineEnd]);
        if (line.len > 0 && line[0] == '*') {
            line = jdblTrimLine(line[1..line.len]);
        }

        if (line.len > 1 && line[0] == '@') {
            let kEnd: usize = 1;
            while (kEnd < line.len && !jdblIsLineWs(line[kEnd])) : (kEnd += 1) {}
            const key = line[1..kEnd];
            let vStart = kEnd;
            while (vStart < line.len && jdblIsLineWs(line[vStart])) : (vStart += 1) {}
            const val = jdblTrimLine(line[vStart..line.len]);
            if (key.len > 0 && written < out.len) {
                out[written] = DocPragma{ key: key, value: val };
                written += 1;
            }
        }

        if (lineEnd >= body.len) {
            break;
        }
        pos = lineEnd + 1;
    }
    return written;
}

pub fn printDocblock(pragmas: []const DocPragma, out: []u8): usize {
    if (pragmas.len == 0) {
        return 0;
    }
    let pos = jdblWriteStr("/**\n", out, 0);
    let i: usize = 0;
    while (i < pragmas.len) : (i += 1) {
        const p = pragmas[i];
        pos = jdblWriteStr(" * @", out, pos);
        pos = jdblWriteStr(p.key, out, pos);
        if (p.value.len > 0) {
            pos = jdblWriteStr(" ", out, pos);
            pos = jdblWriteStr(p.value, out, pos);
        }
        pos = jdblWriteStr("\n", out, pos);
    }
    pos = jdblWriteStr(" */", out, pos);
    return pos;
}
