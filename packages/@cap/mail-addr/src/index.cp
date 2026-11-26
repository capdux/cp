// @cap/mail-addr — Zero-allocation RFC 5322 email address & recipient list parser/formatter for CP
// Clean-room MIT implementation inspired by npm `addressparser` / `email-addresses` (10M/wk) and `nodemailer`.

const MAX_ADDRESSES: usize = 32;

pub struct MailAddress {
    pub valid: bool,
    pub display_name: string,
    pub address: string,
    pub local: string,
    pub domain: string,
}

pub struct AddressList {
    pub items: [MAX_ADDRESSES]MailAddress,
    pub count: usize,

    pub fn get(idx: usize) -> MailAddress? {
        if (idx >= _.count) {
            return null;
        }
        return _.items[idx];
    }
}

fn invalidMail() -> MailAddress {
    return MailAddress{
        valid: false,
        display_name: "",
        address: "",
        local: "",
        domain: "",
    };
}

fn isSpace(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimSpaces(s: string) -> string {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

// Strip leading and trailing whitespace and balanced (...) comments (outside quotes)
fn stripEdgeComments(raw: string) -> string {
    let s = trimSpaces(raw);
    let changed = true;
    while (changed && s.len > 0) {
        changed = false;
        // Strip leading (comment)
        if (s.len > 0 && s[0] == '(') {
            let depth: usize = 0;
            let i: usize = 0;
            let esc = false;
            let closed = false;
            while (i < s.len) {
                const c = s[i];
                if (esc) {
                    esc = false;
                } else if (c == '\\') {
                    esc = true;
                } else if (c == '(') {
                    depth += 1;
                } else if (c == ')') {
                    depth -= 1;
                    if (depth == 0) {
                        s = trimSpaces(s[(i + 1)..s.len]);
                        closed = true;
                        changed = true;
                        break;
                    }
                }
                i += 1;
            }
            if (!closed) {
                break;
            }
        }
        // Strip trailing (comment)
        if (s.len > 0 && s[s.len - 1] == ')') {
            let depth: usize = 0;
            let idx: usize = s.len;
            let opened = false;
            while (idx > 0) {
                idx -= 1;
                const c = s[idx];
                // Check if escaped by preceding backslash
                if (idx > 0 && s[idx - 1] == '\\') {
                    continue;
                }
                if (c == ')') {
                    depth += 1;
                } else if (c == '(') {
                    depth -= 1;
                    if (depth == 0) {
                        s = trimSpaces(s[0..idx]);
                        opened = true;
                        changed = true;
                        break;
                    }
                }
            }
            if (!opened) {
                break;
            }
        }
    }
    return s;
}

fn isValidLocal(local: string) -> bool {
    if (local.len == 0) {
        return false;
    }
    if (local[0] == '.' || local[local.len - 1] == '.') {
        return false;
    }
    let prevDot = false;
    let i: usize = 0;
    while (i < local.len) {
        const c = local[i];
        if (c <= 32 || c >= 127) {
            return false;
        }
        if (c == '@' || c == '<' || c == '>' || c == ',' || c == ';' || c == '"' || c == '(' || c == ')' || c == '\\') {
            return false;
        }
        if (c == '.') {
            if (prevDot) {
                return false;
            }
            prevDot = true;
        } else {
            prevDot = false;
        }
        i += 1;
    }
    return true;
}

fn isValidDomain(domain: string) -> bool {
    if (domain.len < 3) {
        return false;
    }
    if (domain[0] == '.' || domain[domain.len - 1] == '.' || domain[0] == '-' || domain[domain.len - 1] == '-') {
        return false;
    }
    let hasDot = false;
    let prevDot = false;
    let i: usize = 0;
    while (i < domain.len) {
        const c = domain[i];
        const ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '.';
        if (!ok) {
            return false;
        }
        if (c == '.') {
            if (prevDot) {
                return false;
            }
            hasDot = true;
            prevDot = true;
        } else {
            prevDot = false;
        }
        i += 1;
    }
    return hasDot;
}

fn makeAddrFromSpec(displayName: string, addrSpec: string) -> MailAddress {
    const spec = trimSpaces(addrSpec);
    if (spec.len == 0) {
        return invalidMail();
    }
    let atIdx: usize = 0;
    let atCount: usize = 0;
    let i: usize = 0;
    while (i < spec.len) {
        if (spec[i] == '@') {
            atIdx = i;
            atCount += 1;
        }
        i += 1;
    }
    if (atCount != 1) {
        return invalidMail();
    }
    const local = spec[0..atIdx];
    const domain = spec[(atIdx + 1)..spec.len];
    if (!isValidLocal(local) || !isValidDomain(domain)) {
        return invalidMail();
    }
    return MailAddress{
        valid: true,
        display_name: displayName,
        address: spec,
        local: local,
        domain: domain,
    };
}

fn unwrapDisplayName(rawName: string) -> string {
    const cleaned = stripEdgeComments(rawName);
    if (cleaned.len >= 2 && cleaned[0] == '"' && cleaned[cleaned.len - 1] == '"') {
        // Ensure the opening quote actually matches the final quote
        let i: usize = 1;
        let esc = false;
        let closedAt: usize = 0;
        while (i < cleaned.len) {
            const c = cleaned[i];
            if (esc) {
                esc = false;
            } else if (c == '\\') {
                esc = true;
            } else if (c == '"') {
                closedAt = i;
                break;
            }
            i += 1;
        }
        if (closedAt == cleaned.len - 1) {
            return cleaned[1..(cleaned.len - 1)];
        }
    }
    return cleaned;
}

pub fn parseMailbox(raw: string) -> MailAddress {
    const trimmed = trimSpaces(raw);
    if (trimmed.len == 0) {
        return invalidMail();
    }

    // Scan for angle-bracket <addr> outside quotes and comments
    let inQuote = false;
    let escaped = false;
    let commentDepth: usize = 0;
    let ltIdx: usize = 0;
    let gtIdx: usize = 0;
    let hasLt = false;
    let hasGt = false;

    let i: usize = 0;
    while (i < trimmed.len) {
        const c = trimmed[i];
        if (inQuote) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inQuote = false;
            }
        } else if (commentDepth > 0) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '(') {
                commentDepth += 1;
            } else if (c == ')') {
                commentDepth -= 1;
            }
        } else {
            if (c == '"') {
                inQuote = true;
            } else if (c == '(') {
                commentDepth = 1 as usize;
            } else if (c == '<') {
                if (hasLt) {
                    return invalidMail();
                }
                hasLt = true;
                ltIdx = i;
            } else if (c == '>') {
                if (!hasLt || hasGt) {
                    return invalidMail();
                }
                hasGt = true;
                gtIdx = i;
            }
        }
        i += 1;
    }

    if (inQuote || commentDepth > 0) {
        return invalidMail();
    }

    if (hasLt || hasGt) {
        if (!hasLt || !hasGt || gtIdx <= ltIdx + 1) {
            return invalidMail();
        }
        // Only whitespace/comments allowed after '>'
        const trailing = stripEdgeComments(trimmed[(gtIdx + 1)..trimmed.len]);
        if (trailing.len != 0) {
            return invalidMail();
        }
        const namePart = unwrapDisplayName(trimmed[0..ltIdx]);
        const addrPart = trimmed[(ltIdx + 1)..gtIdx];
        return makeAddrFromSpec(namePart, addrPart);
    }

    // Plain addr-spec (strip outer comments like "alice@example.com (Alice)")
    const plain = stripEdgeComments(trimmed);
    return makeAddrFromSpec("", plain);
}

pub fn parseAddressList(raw: string) -> AddressList {
    let items: [MAX_ADDRESSES]MailAddress = undefined;
    let k: usize = 0;
    while (k < MAX_ADDRESSES) {
        items[k] = invalidMail();
        k += 1;
    }

    let count: usize = 0;
    let segStart: usize = 0;
    let inQuote = false;
    let escaped = false;
    let commentDepth: usize = 0;
    let inAngle = false;

    let i: usize = 0;
    while (i <= raw.len) {
        const atEnd = i == raw.len;
        let isComma = false;
        if (!atEnd) {
            const c = raw[i];
            if (inQuote) {
                if (escaped) {
                    escaped = false;
                } else if (c == '\\') {
                    escaped = true;
                } else if (c == '"') {
                    inQuote = false;
                }
            } else if (commentDepth > 0) {
                if (escaped) {
                    escaped = false;
                } else if (c == '\\') {
                    escaped = true;
                } else if (c == '(') {
                    commentDepth += 1;
                } else if (c == ')') {
                    commentDepth -= 1;
                }
            } else {
                if (c == '"') {
                    inQuote = true;
                } else if (c == '(') {
                    commentDepth = 1 as usize;
                } else if (c == '<') {
                    inAngle = true;
                } else if (c == '>') {
                    inAngle = false;
                } else if (c == ',' && !inAngle) {
                    isComma = true;
                }
            }
        }

        if (atEnd || isComma) {
            const seg = trimSpaces(raw[segStart..i]);
            if (seg.len > 0 && count < MAX_ADDRESSES) {
                items[count] = parseMailbox(seg);
                count += 1;
            }
            segStart = i + 1;
        }
        i += 1;
    }

    return AddressList{
        items: items,
        count: count,
    };
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn needsQuoting(name: string) -> bool {
    let i: usize = 0;
    while (i < name.len) {
        const c = name[i];
        if (c == ',' || c == ';' || c == '"' || c == '\\' || c == '<' || c == '>' || c == '(' || c == ')' || c == '@' || c == ':' || c == '.' || c == '[' || c == ']') {
            return true;
        }
        i += 1;
    }
    return false;
}

pub fn formatMailbox(m: MailAddress, out: []u8) -> string {
    if (!m.valid || m.address.len == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    if (m.display_name.len == 0) {
        pos = appendStr(out, pos, m.address);
        return out[0..pos];
    }

    if (needsQuoting(m.display_name)) {
        pos = appendByte(out, pos, '"');
        let i: usize = 0;
        while (i < m.display_name.len && pos < out.len) {
            const c = m.display_name[i];
            if (c == '"' || c == '\\') {
                pos = appendByte(out, pos, '\\');
            }
            pos = appendByte(out, pos, c);
            i += 1;
        }
        pos = appendByte(out, pos, '"');
    } else {
        pos = appendStr(out, pos, m.display_name);
    }

    pos = appendStr(out, pos, " <");
    pos = appendStr(out, pos, m.address);
    pos = appendByte(out, pos, '>');
    return out[0..pos];
}

export {
    MailAddress,
    AddressList,
    parseMailbox,
    parseAddressList,
    formatMailbox,
};
