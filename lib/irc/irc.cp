// Origin: Pure-CP Stdlib (Zero-Alloc RFC 1459 / RFC 2812 / IRCv3 Wire Protocol & Message Tags Codec) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// Protocol Constants & Common Numeric Replies (RFC 1459 / RFC 2812 / IRCv3)
// ---------------------------------------------------------------------------

pub const IRC_TCP_PORT: u16 = 6667;
pub const IRCS_TCP_PORT: u16 = 6697;
pub const IRC_MAX_LINE_LEN: usize = 512;
pub const IRCV3_MAX_TAGS_LEN: usize = 8191;

pub const RPL_WELCOME: u16 = 1;
pub const RPL_YOURHOST: u16 = 2;
pub const RPL_CREATED: u16 = 3;
pub const RPL_MYINFO: u16 = 4;
pub const RPL_ISUPPORT: u16 = 5;
pub const RPL_TOPIC: u16 = 332;
pub const RPL_NAMREPLY: u16 = 353;
pub const RPL_ENDOFNAMES: u16 = 366;
pub const RPL_MOTD: u16 = 372;
pub const RPL_ENDOFMOTD: u16 = 376;
pub const ERR_NOSUCHNICK: u16 = 401;
pub const ERR_NOSUCHCHANNEL: u16 = 403;
pub const ERR_NICKNAMEINUSE: u16 = 433;

// ---------------------------------------------------------------------------
// Internal Byte Helpers
// ---------------------------------------------------------------------------

fn isAsciiAlpha(b: u8): bool {
    return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z');
}

fn isAsciiDigit(b: u8): bool {
    return b >= '0' && b <= '9';
}

fn bytesEq(a: []const u8, b: []const u8): bool {
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

fn isValidCommand(cmd: []const u8): bool {
    if (cmd.len == 0) {
        return false;
    }
    if (cmd.len == 3 && isAsciiDigit(cmd[0]) && isAsciiDigit(cmd[1]) && isAsciiDigit(cmd[2])) {
        return true;
    }
    let i: usize = 0;
    while (i < cmd.len) {
        if (!isAsciiAlpha(cmd[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn containsCrLf(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '\r' || s[i] == '\n') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn containsSpaceOrCrLf(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b == ' ' || b == '\r' || b == '\n') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn isValidTagKey(key: []const u8): bool {
    if (key.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < key.len) {
        const b = key[i];
        if (b == ' ' || b == ';' || b == '=' || b == '\r' || b == '\n') {
            return false;
        }
        i += 1;
    }
    return true;
}

// ---------------------------------------------------------------------------
// IRCv3 Message Tags
// ---------------------------------------------------------------------------

/// A single zero-copy IRCv3 message tag (`key`, optional raw escaped `value`).
pub struct IrcTag {
    key: []const u8,
    value: []const u8,
    has_value: bool,

    /// Returns true if this is a client-only tag (key starts with `+`).
    fn isClientOnly(): bool {
        return _.key.len > 0 && _.key[0] == '+';
    }
}

/// Zero-allocation iterator over semicolon-delimited IRCv3 message tags.
pub struct IrcTagIter {
    raw: []const u8,
    offset: usize,

    fn init(tags_raw: []const u8): IrcTagIter {
        let start: usize = 0;
        if (tags_raw.len > 0 && tags_raw[0] == '@') {
            start = 1;
        }
        return IrcTagIter {
            raw: tags_raw,
            offset: start,
        };
    }

    fn next(*_): IrcTag? {
        while (_.offset < _.raw.len) {
            // Skip empty segments caused by consecutive semicolons.
            if (_.raw[_.offset] == ';') {
                _.offset += 1;
                continue;
            }
            const seg_start = _.offset;
            let eq_pos: usize? = null;
            while (_.offset < _.raw.len && _.raw[_.offset] != ';') {
                if (_.raw[_.offset] == '=' && eq_pos == null) {
                    eq_pos = _.offset;
                }
                _.offset += 1;
            }
            const seg_end = _.offset;
            if (_.offset < _.raw.len && _.raw[_.offset] == ';') {
                _.offset += 1;
            }

            if (eq := eq_pos) {
                const key = _.raw[seg_start..eq];
                if (key.len == 0) {
                    continue;
                }
                return IrcTag {
                    key: key,
                    value: _.raw[eq + 1..seg_end],
                    has_value: true,
                };
            } else {
                const key = _.raw[seg_start..seg_end];
                if (key.len == 0) {
                    continue;
                }
                return IrcTag {
                    key: key,
                    value: "",
                    has_value: false,
                };
            }
        }
        return null;
    }
}

/// Decodes an IRCv3 escaped tag value into caller-provided `out` buffer.
/// Mapping: `\:` -> `;`, `\s` -> ` `, `\\` -> `\`, `\r` -> CR, `\n` -> LF.
/// Unknown escape sequences drop the backslash per IRCv3 spec; trailing lone `\` is ignored.
/// Returns the number of bytes written, or `null` if `out` is too small.
pub fn unescapeTagValue(raw: []const u8, out: []u8): usize? {
    let i: usize = 0;
    let w: usize = 0;
    while (i < raw.len) {
        const b = raw[i];
        if (b == '\\') {
            i += 1;
            if (i >= raw.len) {
                // Lone trailing backslash is discarded per IRCv3 specification.
                break;
            }
            const esc = raw[i];
            let decoded: u8 = esc;
            if (esc == ':') {
                decoded = ';';
            } else if (esc == 's') {
                decoded = ' ';
            } else if (esc == '\\') {
                decoded = '\\';
            } else if (esc == 'r') {
                decoded = '\r';
            } else if (esc == 'n') {
                decoded = '\n';
            }
            if (w >= out.len) {
                return null;
            }
            out[w] = decoded;
            w += 1;
            i += 1;
        } else {
            if (w >= out.len) {
                return null;
            }
            out[w] = b;
            w += 1;
            i += 1;
        }
    }
    return w;
}

/// Escapes a raw IRCv3 tag value into caller-provided `out` buffer.
/// Mapping: `;` -> `\:`, ` ` -> `\s`, `\` -> `\\`, CR -> `\r`, LF -> `\n`.
/// Returns the number of bytes written, or `null` if `out` is too small.
pub fn escapeTagValue(raw: []const u8, out: []u8): usize? {
    let i: usize = 0;
    let w: usize = 0;
    while (i < raw.len) {
        const b = raw[i];
        if (b == ';' || b == ' ' || b == '\\' || b == '\r' || b == '\n') {
            if (w + 2 > out.len) {
                return null;
            }
            out[w] = '\\';
            if (b == ';') {
                out[w + 1] = ':';
            } else if (b == ' ') {
                out[w + 1] = 's';
            } else if (b == '\\') {
                out[w + 1] = '\\';
            } else if (b == '\r') {
                out[w + 1] = 'r';
            } else {
                out[w + 1] = 'n';
            }
            w += 2;
        } else {
            if (w >= out.len) {
                return null;
            }
            out[w] = b;
            w += 1;
        }
        i += 1;
    }
    return w;
}

// ---------------------------------------------------------------------------
// Prefix (`nick!user@host` or `servername`)
// ---------------------------------------------------------------------------

/// Parsed IRC message prefix (`nick!user@host` or `servername`).
pub struct IrcPrefix {
    raw: []const u8,
    nick: []const u8,
    user: []const u8,
    host: []const u8,

    /// Returns true if the prefix has no `!user` and no `@host` component.
    fn isServer(): bool {
        return _.user.len == 0 && _.host.len == 0;
    }

    /// Returns true if the prefix carries user or host components.
    fn isUser(): bool {
        return _.user.len > 0 || _.host.len > 0;
    }
}

/// Splits a raw prefix slice (without leading `:`) into `nick`, `user`, and `host`.
pub fn parseIrcPrefix(raw: []const u8): IrcPrefix {
    let bang_pos: usize? = null;
    let at_pos: usize? = null;
    let i: usize = 0;
    while (i < raw.len) {
        const b = raw[i];
        if (b == '!' && bang_pos == null && at_pos == null) {
            bang_pos = i;
        } else if (b == '@' && at_pos == null) {
            at_pos = i;
        }
        i += 1;
    }

    if (bang := bang_pos) {
        if (at := at_pos) {
            return IrcPrefix {
                raw: raw,
                nick: raw[0..bang],
                user: raw[bang + 1..at],
                host: raw[at + 1..],
            };
        }
        return IrcPrefix {
            raw: raw,
            nick: raw[0..bang],
            user: raw[bang + 1..],
            host: "",
        };
    }

    if (at := at_pos) {
        return IrcPrefix {
            raw: raw,
            nick: raw[0..at],
            user: "",
            host: raw[at + 1..],
        };
    }

    return IrcPrefix {
        raw: raw,
        nick: raw,
        user: "",
        host: "",
    };
}

// ---------------------------------------------------------------------------
// Parameters Iterator
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over IRC command parameters.
/// Splits on ASCII spaces; a token beginning with `:` yields the remainder as the final trailing parameter.
pub struct IrcParamIter {
    raw: []const u8,
    offset: usize,
    done: bool,

    fn init(params_raw: []const u8): IrcParamIter {
        return IrcParamIter {
            raw: params_raw,
            offset: 0,
            done: false,
        };
    }

    fn next(*_): ([]const u8)? {
        if (_.done) {
            return null;
        }
        while (_.offset < _.raw.len && _.raw[_.offset] == ' ') {
            _.offset += 1;
        }
        if (_.offset >= _.raw.len) {
            _.done = true;
            return null;
        }
        if (_.raw[_.offset] == ':') {
            const start = _.offset + 1;
            _.offset = _.raw.len;
            _.done = true;
            return _.raw[start..];
        }
        const start = _.offset;
        while (_.offset < _.raw.len && _.raw[_.offset] != ' ') {
            _.offset += 1;
        }
        return _.raw[start.._.offset];
    }
}

// ---------------------------------------------------------------------------
// IRC Message & Line Parser
// ---------------------------------------------------------------------------

/// Zero-copy view of a parsed RFC 1459 / RFC 2812 / IRCv3 message line.
pub struct IrcMessage {
    tags_raw: []const u8,
    prefix_raw: []const u8,
    command: []const u8,
    params_raw: []const u8,

    fn hasTags(): bool {
        return _.tags_raw.len > 0;
    }

    fn hasPrefix(): bool {
        return _.prefix_raw.len > 0;
    }

    fn tags(): IrcTagIter {
        return IrcTagIter.init(_.tags_raw);
    }

    fn prefix(): IrcPrefix? {
        if (_.prefix_raw.len == 0) {
            return null;
        }
        return parseIrcPrefix(_.prefix_raw);
    }

    fn params(): IrcParamIter {
        return IrcParamIter.init(_.params_raw);
    }

    /// Returns the numeric reply code (`0..999`) if `command` is a 3-digit ASCII number.
    fn numericCode(): u16? {
        if (_.command.len != 3) {
            return null;
        }
        if (!isAsciiDigit(_.command[0]) || !isAsciiDigit(_.command[1]) || !isAsciiDigit(_.command[2])) {
            return null;
        }
        const d0 = (_.command[0] - '0') as u16;
        const d1 = (_.command[1] - '0') as u16;
        const d2 = (_.command[2] - '0') as u16;
        return d0 * 100 + d1 * 10 + d2;
    }

    fn isNumeric(): bool {
        return _.numericCode() != null;
    }

    /// Returns the trailing parameter (text after the `:`-prefixed final parameter), or `null` if none.
    fn trailing(): ([]const u8)? {
        let i: usize = 0;
        while (i < _.params_raw.len) {
            while (i < _.params_raw.len && _.params_raw[i] == ' ') {
                i += 1;
            }
            if (i >= _.params_raw.len) {
                return null;
            }
            if (_.params_raw[i] == ':') {
                return _.params_raw[i + 1..];
            }
            while (i < _.params_raw.len && _.params_raw[i] != ' ') {
                i += 1;
            }
        }
        return null;
    }

    /// Looks up the first IRCv3 tag matching `key`.
    fn findTag(key: []const u8): IrcTag? {
        let it = _.tags();
        while (tag := it.next()) {
            if (bytesEq(tag.key, key)) {
                return tag;
            }
        }
        return null;
    }
}

/// Parses a single IRC / IRCv3 wire line into a zero-copy `IrcMessage`.
/// Automatically strips a trailing `\r\n` or `\n`.
/// Rejects empty lines, empty `@` tags, empty `:` prefixes, missing commands, or invalid commands.
pub fn parseIrcLine(line: []const u8): IrcMessage? {
    let end = line.len;
    if (end > 0 && line[end - 1] == '\n') {
        end -= 1;
        if (end > 0 && line[end - 1] == '\r') {
            end -= 1;
        }
    }
    if (end == 0) {
        return null;
    }

    const s = line[0..end];
    let pos: usize = 0;
    let tags_raw: []const u8 = "";
    let prefix_raw: []const u8 = "";

    // Optional IRCv3 @tags section
    if (s[pos] == '@') {
        let sp = pos + 1;
        while (sp < s.len && s[sp] != ' ') {
            sp += 1;
        }
        if (sp == s.len) {
            // Only @tags with no command
            return null;
        }
        tags_raw = s[pos + 1..sp];
        if (tags_raw.len == 0 || tags_raw.len > IRCV3_MAX_TAGS_LEN) {
            return null;
        }
        pos = sp;
        while (pos < s.len && s[pos] == ' ') {
            pos += 1;
        }
        if (pos >= s.len) {
            return null;
        }
    }

    // Optional :prefix section
    if (s[pos] == ':') {
        let sp = pos + 1;
        while (sp < s.len && s[sp] != ' ') {
            sp += 1;
        }
        if (sp == s.len) {
            // Only :prefix with no command
            return null;
        }
        prefix_raw = s[pos + 1..sp];
        if (prefix_raw.len == 0) {
            return null;
        }
        pos = sp;
        while (pos < s.len && s[pos] == ' ') {
            pos += 1;
        }
        if (pos >= s.len) {
            return null;
        }
    }

    // Mandatory command token
    const cmd_start = pos;
    while (pos < s.len && s[pos] != ' ') {
        pos += 1;
    }
    const command = s[cmd_start..pos];
    if (!isValidCommand(command)) {
        return null;
    }

    // Skip spaces separating command and parameters
    while (pos < s.len && s[pos] == ' ') {
        pos += 1;
    }
    const params_raw = s[pos..];

    return IrcMessage {
        tags_raw: tags_raw,
        prefix_raw: prefix_raw,
        command: command,
        params_raw: params_raw,
    };
}

// ---------------------------------------------------------------------------
// Zero-Alloc IRC Wire Writer
// ---------------------------------------------------------------------------

/// Zero-allocation IRC / IRCv3 wire line encoder writing directly into a caller-supplied buffer.
/// Order of calls: `writeTag*` -> `writePrefix?` -> `writeCommand`/`writeNumeric` -> `writeParam*` -> `writeTrailing?` -> `finish`.
pub struct IrcWriter {
    buf: []u8,
    pos: usize = 0,
    has_tags: bool = false,
    has_prefix: bool = false,
    has_command: bool = false,
    has_trailing: bool = false,
    finished: bool = false,
    failed: bool = false,

    pub fn init(buf: []u8): IrcWriter {
        return IrcWriter {
            buf: buf,
            pos: 0,
            has_tags: false,
            has_prefix: false,
            has_command: false,
            has_trailing: false,
            finished: false,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    fn pushByte(*_, b: u8): bool {
        if (_.pos >= _.buf.len) {
            _.failed = true;
            return false;
        }
        _.buf[_.pos] = b;
        _.pos += 1;
        return true;
    }

    fn pushBytes(*_, s: []const u8): bool {
        if (_.buf.len - _.pos < s.len) {
            _.failed = true;
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.pos + i] = s[i];
            i += 1;
        }
        _.pos += s.len;
        return true;
    }

    /// Appends an IRCv3 message tag before prefix/command.
    /// When `value` is non-null, it is automatically escaped using IRCv3 tag escaping rules.
    fn writeTag(*_, key: []const u8, value: ([]const u8)?): bool {
        if (_.failed || _.finished || _.has_prefix || _.has_command || !isValidTagKey(key)) {
            _.failed = true;
            return false;
        }
        const lead: u8 = if (_.has_tags) ';' else '@';
        if (!_.pushByte(lead)) {
            return false;
        }
        if (!_.pushBytes(key)) {
            return false;
        }
        if (val := value) {
            if (!_.pushByte('=')) {
                return false;
            }
            if (n := escapeTagValue(val, _.buf[_.pos..])) {
                _.pos += n;
            } else {
                _.failed = true;
                return false;
            }
        }
        _.has_tags = true;
        return true;
    }

    /// Writes the optional `:prefix` section (before command).
    fn writePrefix(*_, prefix: []const u8): bool {
        if (_.failed || _.finished || _.has_prefix || _.has_command || prefix.len == 0 || containsSpaceOrCrLf(prefix)) {
            _.failed = true;
            return false;
        }
        if (_.has_tags) {
            if (!_.pushByte(' ')) {
                return false;
            }
        }
        if (!_.pushByte(':') || !_.pushBytes(prefix)) {
            return false;
        }
        _.has_prefix = true;
        return true;
    }

    /// Writes the mandatory command token (ASCII letters or 3-digit numeric).
    fn writeCommand(*_, command: []const u8): bool {
        if (_.failed || _.finished || _.has_command || !isValidCommand(command)) {
            _.failed = true;
            return false;
        }
        if (_.has_tags || _.has_prefix) {
            if (!_.pushByte(' ')) {
                return false;
            }
        }
        if (!_.pushBytes(command)) {
            return false;
        }
        _.has_command = true;
        return true;
    }

    /// Writes a 3-digit numeric reply command (`0..999`, zero-padded e.g. `001`).
    fn writeNumeric(*_, code: u16): bool {
        if (code > 999) {
            _.failed = true;
            return false;
        }
        let digits: [3]u8 = [
            ('0' as u8) + ((code / 100) as u8),
            ('0' as u8) + (((code / 10) % 10) as u8),
            ('0' as u8) + ((code % 10) as u8),
        ];
        return _.writeCommand(digits[0..]);
    }

    /// Appends a non-trailing middle parameter (must not be empty, start with `:`, or contain space/CR/LF).
    fn writeParam(*_, param: []const u8): bool {
        if (_.failed || _.finished || !_.has_command || _.has_trailing || param.len == 0 || param[0] == ':' || containsSpaceOrCrLf(param)) {
            _.failed = true;
            return false;
        }
        if (!_.pushByte(' ') || !_.pushBytes(param)) {
            return false;
        }
        return true;
    }

    /// Appends the final `:<trailing>` parameter (may contain spaces or be empty, must not contain CR/LF).
    fn writeTrailing(*_, text: []const u8): bool {
        if (_.failed || _.finished || !_.has_command || _.has_trailing || containsCrLf(text)) {
            _.failed = true;
            return false;
        }
        if (!_.pushByte(' ') || !_.pushByte(':') || !_.pushBytes(text)) {
            return false;
        }
        _.has_trailing = true;
        return true;
    }

    /// Appends `\r\n` and returns the complete encoded IRC line, or `null` if an error occurred.
    fn finish(*_): ([]const u8)? {
        if (_.failed || !_.has_command) {
            _.failed = true;
            return null;
        }
        if (!_.finished) {
            if (!_.pushByte('\r') || !_.pushByte('\n')) {
                return null;
            }
            _.finished = true;
        }
        return _.buf[0.._.pos];
    }
}
