// Origin: Pure-CP Stdlib (Zero-Alloc RFC 959 / RFC 2428 / RFC 3659 FTP Control & MLSD Codec) | Created: 2026-11-22

pub const FTP_CONTROL_PORT: u16 = 21;
pub const FTP_DATA_PORT: u16 = 20;
pub const FTPS_IMPLICIT_PORT: u16 = 990;

// Standard FTP Reply Codes (RFC 959 + RFC 2428)
pub const RPL_DATA_OPEN: u16 = 125;
pub const RPL_FILE_OK: u16 = 150;
pub const RPL_OK: u16 = 200;
pub const RPL_READY: u16 = 220;
pub const RPL_CLOSING: u16 = 221;
pub const RPL_TRANSFER_OK: u16 = 226;
pub const RPL_PASV: u16 = 227;
pub const RPL_EPSV: u16 = 229;
pub const RPL_LOGGED_IN: u16 = 230;
pub const RPL_PATHNAME: u16 = 257;
pub const RPL_NEED_PASSWORD: u16 = 331;
pub const RPL_NOT_LOGGED_IN: u16 = 530;
pub const RPL_FILE_UNAVAILABLE: u16 = 550;

fn stripCrlf(line: []const u8): []const u8 {
    let end = line.len;
    while (end > 0 && (line[end - 1] == '\r' || line[end - 1] == '\n')) {
        end -= 1;
    }
    return line[0..end];
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn asciiEqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn containsCrlf(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '\r' || s[i] == '\n') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn parseU64Dec(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const digit = (c - '0') as u64;
        if (acc > (18446744073709551615 - digit) / 10) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

fn parseU8Dec(s: []const u8): u8? {
    const v = parseU64Dec(s) ?? return null;
    if (v > 255) {
        return null;
    }
    return v as u8;
}

fn writeBytes(out: []u8, pos: usize, src: []const u8): usize? {
    if (pos + src.len > out.len) {
        return null;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeU32Dec(out: []u8, pos: usize, val: u32): usize? {
    let tmp: [10]u8 = [0; 10];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = ('0' + ((v % 10) as u8)) as u8;
            v = v / 10;
            n += 1;
        }
    }
    if (pos + n > out.len) {
        return null;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

pub struct FtpCommand {
    verb: []const u8,
    arg: []const u8,
}

/// Parses an FTP command line (strips trailing CRLF and splits on first ASCII space).
pub fn parseFtpCommand(line: []const u8): FtpCommand? {
    const s = stripCrlf(line);
    if (s.len == 0) {
        return null;
    }
    let sp: usize = 0;
    while (sp < s.len && s[sp] != ' ') {
        sp += 1;
    }
    if (sp == 0) {
        return null;
    }
    const verb = s[0..sp];
    let arg: []const u8 = "";
    if (sp < s.len) {
        arg = s[(sp + 1)..];
    }
    return FtpCommand{
        .verb = verb,
        .arg = arg,
    };
}

pub struct FtpReplyLine {
    code: u16,
    is_continuation: bool,
    message: []const u8,

    fn isPreliminary(): bool {
        return _.code >= 100 && _.code < 200;
    }

    fn isPositive(): bool {
        return _.code >= 200 && _.code < 300;
    }

    fn isIntermediate(): bool {
        return _.code >= 300 && _.code < 400;
    }

    fn isTransientError(): bool {
        return _.code >= 400 && _.code < 500;
    }

    fn isPermanentError(): bool {
        return _.code >= 500 && _.code < 600;
    }
}

/// Parses a single FTP reply line (`XYZ message` or multi-line `XYZ-message`).
pub fn parseFtpReplyLine(line: []const u8): FtpReplyLine? {
    const s = stripCrlf(line);
    if (s.len < 3) {
        return null;
    }
    const d0 = s[0];
    const d1 = s[1];
    const d2 = s[2];
    if (d0 < '1' || d0 > '5' || d1 < '0' || d1 > '9' || d2 < '0' || d2 > '9') {
        return null;
    }
    const code: u16 = ((d0 - '0') as u16) * 100 + ((d1 - '0') as u16) * 10 + ((d2 - '0') as u16);

    if (s.len == 3) {
        return FtpReplyLine{
            .code = code,
            .is_continuation = false,
            .message = "",
        };
    }
    const sep = s[3];
    if (sep != ' ' && sep != '-') {
        return null;
    }
    return FtpReplyLine{
        .code = code,
        .is_continuation = (sep == '-'),
        .message = s[4..],
    };
}

pub struct FtpPasvAddr {
    ip: [4]u8,
    port: u16,
}

/// Parses RFC 959 PASV address `(h1,h2,h3,h4,p1,p2)` from a 227 reply message.
pub fn parsePasvReply(message: []const u8): FtpPasvAddr? {
    let open_idx: usize = 0;
    let found_open = false;
    while (open_idx < message.len) {
        if (message[open_idx] == '(') {
            found_open = true;
            break;
        }
        open_idx += 1;
    }
    if (!found_open) {
        return null;
    }
    let close_idx = open_idx + 1;
    let found_close = false;
    while (close_idx < message.len) {
        if (message[close_idx] == ')') {
            found_close = true;
            break;
        }
        close_idx += 1;
    }
    if (!found_close || close_idx <= open_idx + 1) {
        return null;
    }

    const inner = message[(open_idx + 1)..close_idx];
    let nums: [6]u8 = [0; 6];
    let part: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= inner.len) {
        if (i == inner.len || inner[i] == ',') {
            if (part >= 6) {
                return null;
            }
            const val = parseU8Dec(inner[start..i]) ?? return null;
            nums[part] = val;
            part += 1;
            start = i + 1;
        }
        i += 1;
    }
    if (part != 6) {
        return null;
    }

    const ip: [4]u8 = [nums[0], nums[1], nums[2], nums[3]];
    const port: u16 = ((nums[4] as u16) << 8) | (nums[5] as u16);
    return FtpPasvAddr{
        .ip = ip,
        .port = port,
    };
}

/// Parses RFC 2428 EPSV port `(|||port|)` (or arbitrary printable delimiter `(dddportd)`).
pub fn parseEpsvPort(message: []const u8): u16? {
    let open_idx: usize = 0;
    let found_open = false;
    while (open_idx < message.len) {
        if (message[open_idx] == '(') {
            found_open = true;
            break;
        }
        open_idx += 1;
    }
    if (!found_open) {
        return null;
    }
    let close_idx = open_idx + 1;
    let found_close = false;
    while (close_idx < message.len) {
        if (message[close_idx] == ')') {
            found_close = true;
            break;
        }
        close_idx += 1;
    }
    if (!found_close) {
        return null;
    }

    const inner = message[(open_idx + 1)..close_idx];
    if (inner.len < 5) {
        return null;
    }
    const d = inner[0];
    if (d < 33 || d > 126) {
        return null;
    }
    if (inner[1] != d || inner[2] != d || inner[inner.len - 1] != d) {
        return null;
    }
    const port_str = inner[3..(inner.len - 1)];
    const port_u64 = parseU64Dec(port_str) ?? return null;
    if (port_u64 == 0 || port_u64 > 65535) {
        return null;
    }
    return port_u64 as u16;
}

pub struct FtpMlsxFact {
    name: []const u8,
    value: []const u8,
}

/// Zero-copy iterator over RFC 3659 MLSD/MLST `name=value;` facts.
pub struct FtpMlsxFactIter {
    data: []const u8,
    offset: usize,

    static fn init(facts_raw: []const u8): FtpMlsxFactIter {
        return FtpMlsxFactIter{
            .data = facts_raw,
            .offset = 0,
        };
    }

    fn next(*_): FtpMlsxFact? {
        while (_.offset < _.data.len) {
            let semi = _.offset;
            while (semi < _.data.len && _.data[semi] != ';') {
                semi += 1;
            }
            const item = _.data[_.offset..semi];
            _.offset = if (semi < _.data.len) semi + 1 else _.data.len;
            if (item.len == 0) {
                continue;
            }
            let eq: usize = 0;
            while (eq < item.len && item[eq] != '=') {
                eq += 1;
            }
            if (eq == 0 || eq >= item.len) {
                return null;
            }
            return FtpMlsxFact{
                .name = item[0..eq],
                .value = item[(eq + 1)..],
            };
        }
        return null;
    }
}

pub struct FtpMlsxEntry {
    facts_raw: []const u8,
    filename: []const u8,

    fn facts(): FtpMlsxFactIter {
        return FtpMlsxFactIter.init(_.facts_raw);
    }

    fn findFact(name: []const u8): ([]const u8)? {
        let it = _.facts();
        while (true) {
            if (f := it.next()) {
                if (asciiEqIgnoreCase(f.name, name)) {
                    return f.value;
                }
            } else {
                break;
            }
        }
        return null;
    }

    fn size(): u64? {
        if (v := _.findFact("size")) {
            return parseU64Dec(v);
        }
        return null;
    }

    fn entryType(): ([]const u8)? {
        return _.findFact("type");
    }
}

/// Parses an RFC 3659 MLSD / MLST entry line (`facts; filename`).
pub fn parseMlsxLine(line: []const u8): FtpMlsxEntry? {
    const s = stripCrlf(line);
    let sp: usize = 0;
    while (sp < s.len && s[sp] != ' ') {
        sp += 1;
    }
    if (sp >= s.len || sp + 1 >= s.len) {
        return null;
    }
    return FtpMlsxEntry{
        .facts_raw = s[0..sp],
        .filename = s[(sp + 1)..],
    };
}

/// Zero-alloc encoder for FTP command line (`VERB\r\n` or `VERB ARG\r\n`).
pub fn encodeFtpCommand(verb: []const u8, arg: []const u8, out: []u8): usize? {
    if (verb.len == 0 || containsCrlf(verb) || containsCrlf(arg)) {
        return null;
    }
    let pos = writeBytes(out, 0, verb) ?? return null;
    if (arg.len > 0) {
        pos = writeBytes(out, pos, " ") ?? return null;
        pos = writeBytes(out, pos, arg) ?? return null;
    }
    pos = writeBytes(out, pos, "\r\n") ?? return null;
    return pos;
}

/// Zero-alloc encoder for RFC 959 `PORT h1,h2,h3,h4,p1,p2\r\n`.
pub fn encodePortCommand(ip: [4]u8, port: u16, out: []u8): usize? {
    let pos = writeBytes(out, 0, "PORT ") ?? return null;
    pos = writeU32Dec(out, pos, ip[0] as u32) ?? return null;
    pos = writeBytes(out, pos, ",") ?? return null;
    pos = writeU32Dec(out, pos, ip[1] as u32) ?? return null;
    pos = writeBytes(out, pos, ",") ?? return null;
    pos = writeU32Dec(out, pos, ip[2] as u32) ?? return null;
    pos = writeBytes(out, pos, ",") ?? return null;
    pos = writeU32Dec(out, pos, ip[3] as u32) ?? return null;
    pos = writeBytes(out, pos, ",") ?? return null;
    pos = writeU32Dec(out, pos, ((port >> 8) & 0xFF) as u32) ?? return null;
    pos = writeBytes(out, pos, ",") ?? return null;
    pos = writeU32Dec(out, pos, (port & 0xFF) as u32) ?? return null;
    pos = writeBytes(out, pos, "\r\n") ?? return null;
    return pos;
}

/// Zero-alloc encoder for RFC 2428 `EPRT |1|ipv4|port|\r\n` or `EPRT |2|ipv6|port|\r\n`.
pub fn encodeEprtCommand(is_ipv6: bool, addr: []const u8, port: u16, out: []u8): usize? {
    if (addr.len == 0 || containsCrlf(addr)) {
        return null;
    }
    let i: usize = 0;
    while (i < addr.len) {
        if (addr[i] == '|') {
            return null;
        }
        i += 1;
    }
    let pos = writeBytes(out, 0, if (is_ipv6) "EPRT |2|" else "EPRT |1|") ?? return null;
    pos = writeBytes(out, pos, addr) ?? return null;
    pos = writeBytes(out, pos, "|") ?? return null;
    pos = writeU32Dec(out, pos, port as u32) ?? return null;
    pos = writeBytes(out, pos, "|\r\n") ?? return null;
    return pos;
}
