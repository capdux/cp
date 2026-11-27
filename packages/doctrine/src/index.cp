// @cap/doctrine — Zero-allocation JSDoc comment parser
// Clean-room Apache-2.0 implementation inspired by ESLint npm:doctrine

import { strEq, trim, findByte } from "cap:ascii";

pub const DOC_MAX_TAGS: usize = 16;

pub struct JsDocTag {
    title: string,
    type_expr: string,
    name: string,
    description: string,
    optional: bool,
}

fn docIsSpace(ch: u8): bool {
    return ch == ' ' || ch == '\t';
}

fn docStripLinePrefix(line: string): string {
    let start: usize = 0;
    let end = line.len;
    if (end > 0 && line[end - 1] == '\r') {
        end -= 1;
    }
    while (start < end && docIsSpace(line[start])) {
        start += 1;
    }
    if (start < end && line[start] == '*') {
        start += 1;
        if (start < end && docIsSpace(line[start])) {
            start += 1;
        }
    }
    while (end > start && docIsSpace(line[end - 1])) {
        end -= 1;
    }
    return line[start..end];
}

fn docTagHasName(title: string): bool {
    if (strEq(title, "param")) return true;
    if (strEq(title, "arg")) return true;
    if (strEq(title, "argument")) return true;
    if (strEq(title, "property")) return true;
    if (strEq(title, "prop")) return true;
    if (strEq(title, "template")) return true;
    if (strEq(title, "typedef")) return true;
    return false;
}

fn docParseTagLine(line: string): JsDocTag {
    // line starts with '@'
    let pos: usize = 1;
    const title_start = pos;
    while (pos < line.len && !docIsSpace(line[pos]) && line[pos] != '{') {
        pos += 1;
    }
    const title = line[title_start..pos];

    while (pos < line.len && docIsSpace(line[pos])) {
        pos += 1;
    }

    let type_expr: string = "";
    let optional = false;

    if (pos < line.len && line[pos] == '{') {
        pos += 1;
        const type_start = pos;
        let brace_depth: usize = 1;
        while (pos < line.len && brace_depth > 0) {
            if (line[pos] == '{') {
                brace_depth += 1;
            } else if (line[pos] == '}') {
                brace_depth -= 1;
                if (brace_depth == 0) break;
            }
            pos += 1;
        }
        type_expr = trim(line[type_start..pos]);
        if (type_expr.len > 0 && type_expr[type_expr.len - 1] == '=') {
            optional = true;
        }
        if (pos < line.len && line[pos] == '}') {
            pos += 1;
        }
        while (pos < line.len && docIsSpace(line[pos])) {
            pos += 1;
        }
    }

    let name: string = "";
    if (docTagHasName(title) && pos < line.len) {
        if (line[pos] == '[') {
            optional = true;
            pos += 1;
            const n_start = pos;
            while (pos < line.len && line[pos] != ']') {
                pos += 1;
            }
            const raw_bracket = trim(line[n_start..pos]);
            // Strip optional "=default" inside [name=default]
            const eq_idx = findByte(raw_bracket, '=', 0) ?? raw_bracket.len;
            name = trim(raw_bracket[0..eq_idx]);
            if (pos < line.len && line[pos] == ']') {
                pos += 1;
            }
        } else {
            const n_start = pos;
            while (pos < line.len && !docIsSpace(line[pos])) {
                pos += 1;
            }
            name = line[n_start..pos];
        }
        while (pos < line.len && docIsSpace(line[pos])) {
            pos += 1;
        }
    }

    // Optional separator "- "
    if (pos < line.len && line[pos] == '-') {
        if (pos + 1 == line.len || docIsSpace(line[pos + 1])) {
            pos += 1;
            while (pos < line.len && docIsSpace(line[pos])) {
                pos += 1;
            }
        }
    }

    const desc = trim(line[pos..line.len]);
    return JsDocTag{
        title: title,
        type_expr: type_expr,
        name: name,
        description: desc,
        optional: optional,
    };
}

pub struct JsDocComment {
    description: string,
    tags: [16]JsDocTag,
    tag_count: usize,

    fn tagCount(): usize {
        return _.tag_count;
    }

    fn getTag(idx: usize): JsDocTag? {
        if (idx >= _.tag_count) return null;
        return _.tags[idx];
    }

    fn findTag(title: string): JsDocTag? {
        let i: usize = 0;
        while (i < _.tag_count) : (i += 1) {
            if (strEq(_.tags[i].title, title)) {
                return _.tags[i];
            }
        }
        return null;
    }

    fn isDeprecated(): bool {
        let i: usize = 0;
        while (i < _.tag_count) : (i += 1) {
            if (strEq(_.tags[i].title, "deprecated")) {
                return true;
            }
        }
        return false;
    }
}

pub fn parseJsDoc(comment: string): JsDocComment {
    let tags: [16]JsDocTag = undefined;

    let body = trim(comment);
    if (body.len >= 3 && body[0] == '/' && body[1] == '*' && body[2] == '*') {
        body = body[3..body.len];
    } else if (body.len >= 2 && body[0] == '/' && body[1] == '*') {
        body = body[2..body.len];
    }
    if (body.len >= 2 && body[body.len - 2] == '*' && body[body.len - 1] == '/') {
        body = body[0..(body.len - 2)];
    }

    let description: string = "";
    let has_desc = false;
    let in_tags = false;
    let tag_count: usize = 0;

    let pos: usize = 0;
    while (pos <= body.len) {
        const line_start = pos;
        while (pos < body.len && body[pos] != '\n') {
            pos += 1;
        }
        const raw_line = body[line_start..pos];
        const clean = docStripLinePrefix(raw_line);

        if (clean.len > 0) {
            if (clean[0] == '@') {
                in_tags = true;
                if (tag_count < DOC_MAX_TAGS) {
                    tags[tag_count] = docParseTagLine(clean);
                    tag_count += 1;
                }
            } else if (!in_tags && !has_desc) {
                description = clean;
                has_desc = true;
            }
        }

        if (pos == body.len) break;
        pos += 1;
    }

    return JsDocComment{
        description: description,
        tags: tags,
        tag_count: tag_count,
    };
}
