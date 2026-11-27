// @cap/css-what — Zero-allocation CSS selector parser & serializer in pure CP
// Reference: npm:css-what (https://github.com/fb55/css-what, BSD-2-Clause)

import { isSpace, isAlpha, isDigit } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub enum CssTokenKind {
    Tag,
    Universal,
    Id,
    Class,
    Attribute,
    Pseudo,
    PseudoElement,
    Descendant,
    Child,
    Adjacent,
    Sibling,
    Comma,
}

pub enum AttrAction {
    Exists,
    Equals,
    Start,
    End,
    Any,
    Not,
    Element,
    Hyphen,
}

pub struct CssSelectorToken {
    kind: CssTokenKind,
    name: string,
    value: string,
    action: AttrAction,
    ignore_case: bool,
}

struct CwhWriter {
    buf: []u8,
    pos: usize,
    overflow: bool,

    fn writeByte(*_, b: u8) {
        if (_.overflow) return;
        if (_.pos >= _.buf.len) {
            _.overflow = true;
            return;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
    }

    fn putStr(*_, s: string) {
        if (_.overflow) return;
        if (_.pos + s.len > _.buf.len) {
            _.overflow = true;
            return;
        }
        _.pos = writeStr(s, _.buf, _.pos);
    }
}

fn cwhIsIdentStart(c: u8): bool {
    return isAlpha(c) || c == '_' || c == '-' || c >= 0x80;
}

fn cwhIsIdentChar(c: u8): bool {
    return cwhIsIdentStart(c) || isDigit(c);
}

fn cwhSkipSpace(s: string, start: usize): usize {
    let i = start;
    while (i < s.len && isSpace(s[i])) {
        i += 1;
    }
    return i;
}

fn cwhScanIdent(s: string, start: usize): usize {
    let i = start;
    if (i >= s.len) return start;
    if (s[i] != '\\' && !cwhIsIdentStart(s[i])) return start;
    while (i < s.len) {
        if (s[i] == '\\') {
            if (i + 1 >= s.len) return start;
            i += 2;
        } else if (cwhIsIdentChar(s[i])) {
            i += 1;
        } else {
            break;
        }
    }
    return i;
}

fn cwhIsCombinatorChar(c: u8): bool {
    return c == '>' || c == '+' || c == '~' || c == ',';
}

fn cwhToken(kind: CssTokenKind, name: string, value: string, action: AttrAction, ignore_case: bool): CssSelectorToken {
    return CssSelectorToken{
        kind: kind,
        name: name,
        value: value,
        action: action,
        ignore_case: ignore_case,
    };
}

pub fn parseCssSelector(selector: string, out: []CssSelectorToken): i64 {
    let i = cwhSkipSpace(selector, 0);
    let end = selector.len;
    while (end > i && isSpace(selector[end - 1])) {
        end -= 1;
    }
    if (i >= end) return 0;

    let count: usize = 0;
    let expect_selector = true;
    let had_space = false;

    while (i < end) {
        if (isSpace(selector[i])) {
            had_space = true;
            i = cwhSkipSpace(selector, i);
            continue;
        }

        const ch = selector[i];

        // Explicit combinators & comma
        if (cwhIsCombinatorChar(ch)) {
            if (expect_selector) return -1;
            if (count >= out.len) return -1;
            let k: CssTokenKind = .Comma;
            if (ch == '>') {
                k = .Child;
            } else if (ch == '+') {
                k = .Adjacent;
            } else if (ch == '~') {
                k = .Sibling;
            } else {
                k = .Comma;
            }
            out[count] = cwhToken(k, "", "", .Exists, false);
            count += 1;
            i += 1;
            expect_selector = true;
            had_space = false;
            continue;
        }

        // Descendant combinator if whitespace separated two compound selectors
        if (had_space && !expect_selector) {
            if (count >= out.len) return -1;
            out[count] = cwhToken(.Descendant, "", "", .Exists, false);
            count += 1;
        }
        had_space = false;

        if (ch == '*') {
            if (count >= out.len) return -1;
            out[count] = cwhToken(.Universal, "*", "", .Exists, false);
            count += 1;
            i += 1;
            expect_selector = false;
            continue;
        }

        if (ch == '#' || ch == '.') {
            i += 1;
            const id_end = cwhScanIdent(selector, i);
            if (id_end == i) return -1;
            if (count >= out.len) return -1;
            let k: CssTokenKind = .Id;
            if (ch == '.') k = .Class;
            out[count] = cwhToken(k, selector[i..id_end], "", .Exists, false);
            count += 1;
            i = id_end;
            expect_selector = false;
            continue;
        }

        if (ch == '[') {
            i += 1;
            i = cwhSkipSpace(selector, i);
            const name_start = i;
            const name_end = cwhScanIdent(selector, i);
            if (name_end == name_start) return -1;
            const attr_name = selector[name_start..name_end];
            i = cwhSkipSpace(selector, name_end);
            if (i >= end) return -1;

            if (selector[i] == ']') {
                if (count >= out.len) return -1;
                out[count] = cwhToken(.Attribute, attr_name, "", .Exists, false);
                count += 1;
                i += 1;
                expect_selector = false;
                continue;
            }

            let action: AttrAction = .Equals;
            const op_c = selector[i];
            if (op_c == '=') {
                action = .Equals;
                i += 1;
            } else if (i + 1 < end && selector[i + 1] == '=') {
                if (op_c == '^') {
                    action = .Start;
                } else if (op_c == '$') {
                    action = .End;
                } else if (op_c == '*') {
                    action = .Any;
                } else if (op_c == '!') {
                    action = .Not;
                } else if (op_c == '~') {
                    action = .Element;
                } else if (op_c == '|') {
                    action = .Hyphen;
                } else {
                    return -1;
                }
                i += 2;
            } else {
                return -1;
            }

            i = cwhSkipSpace(selector, i);
            if (i >= end) return -1;

            let val_slice: string = "";
            if (selector[i] == '"' || selector[i] == '\'') {
                const quote = selector[i];
                i += 1;
                const vstart = i;
                let closed = false;
                while (i < end) {
                    if (selector[i] == '\\') {
                        if (i + 1 >= end) return -1;
                        i += 2;
                        continue;
                    }
                    if (selector[i] == quote) {
                        val_slice = selector[vstart..i];
                        i += 1;
                        closed = true;
                        break;
                    }
                    i += 1;
                }
                if (!closed) return -1;
            } else {
                const vstart = i;
                while (i < end && !isSpace(selector[i]) && selector[i] != ']') {
                    if (selector[i] == '\\') {
                        if (i + 1 >= end) return -1;
                        i += 2;
                        continue;
                    }
                    i += 1;
                }
                if (i == vstart) return -1;
                val_slice = selector[vstart..i];
            }

            i = cwhSkipSpace(selector, i);
            let ignore_case = false;
            if (i < end && selector[i] != ']') {
                const flag = selector[i];
                if (flag == 'i' || flag == 'I') {
                    ignore_case = true;
                } else if (flag == 's' || flag == 'S') {
                    ignore_case = false;
                } else {
                    return -1;
                }
                i += 1;
                i = cwhSkipSpace(selector, i);
            }

            if (i >= end || selector[i] != ']') return -1;
            i += 1;

            if (count >= out.len) return -1;
            out[count] = cwhToken(.Attribute, attr_name, val_slice, action, ignore_case);
            count += 1;
            expect_selector = false;
            continue;
        }

        if (ch == ':') {
            let is_elem = false;
            i += 1;
            if (i < end && selector[i] == ':') {
                is_elem = true;
                i += 1;
            }
            const pstart = i;
            const pend = cwhScanIdent(selector, i);
            if (pend == pstart) return -1;
            const pname = selector[pstart..pend];
            i = pend;

            let parg: string = "";
            if (i < end && selector[i] == '(') {
                i += 1;
                const astart = i;
                let depth: i32 = 1;
                let in_quote: u8 = 0;
                while (i < end && depth > 0) {
                    const c = selector[i];
                    if (c == '\\') {
                        if (i + 1 >= end) return -1;
                        i += 2;
                        continue;
                    }
                    if (in_quote != 0) {
                        if (c == in_quote) in_quote = 0;
                    } else {
                        if (c == '"' || c == '\'') {
                            in_quote = c;
                        } else if (c == '(') {
                            depth += 1;
                        } else if (c == ')') {
                            depth -= 1;
                            if (depth == 0) {
                                parg = selector[astart..i];
                                i += 1;
                                break;
                            }
                        }
                    }
                    i += 1;
                }
                if (depth != 0) return -1;
            }

            if (count >= out.len) return -1;
            let k: CssTokenKind = .Pseudo;
            if (is_elem) k = .PseudoElement;
            out[count] = cwhToken(k, pname, parg, .Exists, false);
            count += 1;
            expect_selector = false;
            continue;
        }

        // Tag name
        const tstart = i;
        const tend = cwhScanIdent(selector, i);
        if (tend == tstart) return -1;
        if (count >= out.len) return -1;
        out[count] = cwhToken(.Tag, selector[tstart..tend], "", .Exists, false);
        count += 1;
        i = tend;
        expect_selector = false;
    }

    if (expect_selector) return -1;
    return count as i64;
}

pub fn stringifyCssSelector(tokens: []const CssSelectorToken, out: []u8): usize {
    if (tokens.len == 0) return 0;
    let w = CwhWriter{ buf: out, pos: 0, overflow: false };

    let i: usize = 0;
    while (i < tokens.len) {
        const tok = tokens[i];
        if (tok.kind == .Tag) {
            w.putStr(tok.name);
        } else if (tok.kind == .Universal) {
            w.writeByte('*');
        } else if (tok.kind == .Id) {
            w.writeByte('#');
            w.putStr(tok.name);
        } else if (tok.kind == .Class) {
            w.writeByte('.');
            w.putStr(tok.name);
        } else if (tok.kind == .Attribute) {
            w.writeByte('[');
            w.putStr(tok.name);
            if (tok.action != .Exists) {
                if (tok.action == .Equals) {
                    w.writeByte('=');
                } else if (tok.action == .Start) {
                    w.putStr("^=");
                } else if (tok.action == .End) {
                    w.putStr("$=");
                } else if (tok.action == .Any) {
                    w.putStr("*=");
                } else if (tok.action == .Not) {
                    w.putStr("!=");
                } else if (tok.action == .Element) {
                    w.putStr("~=");
                } else if (tok.action == .Hyphen) {
                    w.putStr("|=");
                }
                w.writeByte('"');
                w.putStr(tok.value);
                w.writeByte('"');
                if (tok.ignore_case) {
                    w.putStr(" i");
                }
            }
            w.writeByte(']');
        } else if (tok.kind == .Pseudo) {
            w.writeByte(':');
            w.putStr(tok.name);
            if (tok.value.len > 0) {
                w.writeByte('(');
                w.putStr(tok.value);
                w.writeByte(')');
            }
        } else if (tok.kind == .PseudoElement) {
            w.putStr("::");
            w.putStr(tok.name);
            if (tok.value.len > 0) {
                w.writeByte('(');
                w.putStr(tok.value);
                w.writeByte(')');
            }
        } else if (tok.kind == .Descendant) {
            w.writeByte(' ');
        } else if (tok.kind == .Child) {
            w.putStr(" > ");
        } else if (tok.kind == .Adjacent) {
            w.putStr(" + ");
        } else if (tok.kind == .Sibling) {
            w.putStr(" ~ ");
        } else if (tok.kind == .Comma) {
            w.putStr(", ");
        }
        i += 1;
    }

    if (w.overflow) return 0;
    return w.pos;
}
