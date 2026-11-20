// Source: cp/lib/tui/input.cp (ANSI/VT100/xterm Keyboard Input Sequence Parser)
// Pure CP zero-allocation ANSI/VT100/xterm/SS3/UTF-8 terminal keyboard sequence parser.

const std = @import("std");

/// Logical keyboard key code produced by `InputParser.parseNext`.
pub enum KeyCode {
    Char: u21,
    Enter,
    Esc,
    Tab,
    BackTab,
    Backspace,
    Up,
    Down,
    Left,
    Right,
    Home,
    End,
    PageUp,
    PageDown,
    Delete,
    F: u8,
    Unknown,

    /// Constructs a `KeyCode.Char` variant (works across both `cap:tui` and `cap.tui`).
    static fn makeChar(ch: u21): KeyCode {
        return KeyCode.Char(ch);
    }

    /// Constructs a `KeyCode.F` function-key variant (works across both `cap:tui` and `cap.tui`).
    static fn makeF(n: u8): KeyCode {
        return KeyCode.F(n);
    }

    /// Returns `true` if this `KeyCode` equals `other`.
    fn eql(other: KeyCode): bool {
        return switch (_) {
            .Char ch => switch (other) {
                .Char o => ch == o,
                else => false,
            },
            .Enter => switch (other) { .Enter => true, else => false },
            .Esc => switch (other) { .Esc => true, else => false },
            .Tab => switch (other) { .Tab => true, else => false },
            .BackTab => switch (other) { .BackTab => true, else => false },
            .Backspace => switch (other) { .Backspace => true, else => false },
            .Up => switch (other) { .Up => true, else => false },
            .Down => switch (other) { .Down => true, else => false },
            .Left => switch (other) { .Left => true, else => false },
            .Right => switch (other) { .Right => true, else => false },
            .Home => switch (other) { .Home => true, else => false },
            .End => switch (other) { .End => true, else => false },
            .PageUp => switch (other) { .PageUp => true, else => false },
            .PageDown => switch (other) { .PageDown => true, else => false },
            .Delete => switch (other) { .Delete => true, else => false },
            .F n => switch (other) {
                .F o => n == o,
                else => false,
            },
            .Unknown => switch (other) { .Unknown => true, else => false },
        };
    }
}

/// Modifier key bitflags associated with a `KeyEvent`.
pub struct KeyModifiers {
    ctrl: bool = false;
    alt: bool = false;
    shift: bool = false;

    /// Returns empty modifiers (`ctrl=false, alt=false, shift=false`).
    static fn none(): KeyModifiers {
        return KeyModifiers{ ctrl: false, alt: false, shift: false };
    }

    /// Decodes an xterm CSI modifier parameter (1-based: `1 + bitmask`).
    static fn fromXtermParam(param: u32): KeyModifiers {
        if (param <= 1) return KeyModifiers.none();
        const mask = param - 1;
        return KeyModifiers{
            ctrl: (mask & 4) != 0,
            alt: (mask & 2) != 0,
            shift: (mask & 1) != 0,
        };
    }

    /// Returns `true` if all modifier flags match `other`.
    fn eql(other: KeyModifiers): bool {
        return _.ctrl == other.ctrl && _.alt == other.alt && _.shift == other.shift;
    }
}

/// Decoded terminal keyboard event with consumed byte count for streaming buffers.
pub struct KeyEvent {
    code: KeyCode;
    mods: KeyModifiers = KeyModifiers{};
    consumed_bytes: usize = 1;

    /// Returns `true` if `code`, `mods`, and `consumed_bytes` all match `other`.
    fn eql(other: KeyEvent): bool {
        return _.code.eql(other.code) && _.mods.eql(other.mods) && _.consumed_bytes == other.consumed_bytes;
    }
}

struct Utf8Decoded {
    cp: u21;
    len: usize;
}

fn decodeUtf8At(bytes: []const u8): Utf8Decoded? {
    if (bytes.len == 0) return null;
    const seq_len_u3 = std.unicode.utf8ByteSequenceLength(bytes[0]) catch return null;
    const seq_len: usize = seq_len_u3;
    if (seq_len == 0 || seq_len > bytes.len) return null;
    const cp = std.unicode.utf8Decode(bytes[0..seq_len]) catch return null;
    return Utf8Decoded{ cp: cp, len: seq_len };
}

fn parseCsiSequence(bytes: []const u8): KeyEvent {
    // bytes[0] == 0x1B, bytes[1] == '['
    const n = bytes.len;
    let i: usize = 2;

    // Optional private/intermediate prefix ('?', '>', '<', '=')
    let is_private = false;
    if (i < n && (bytes[i] == '?' || bytes[i] == '>' || bytes[i] == '<' || bytes[i] == '=')) {
        is_private = true;
        i += 1;
    }

    // Parse up to 4 semicolon-separated numeric parameters
    let params: [4]u32 = [0, 0, 0, 0];
    let has_param: [4]bool = [false, false, false, false];
    let p_count: usize = 0;

    while (i < n) {
        const b = bytes[i];
        if (b >= '0' && b <= '9') {
            if (p_count == 0) p_count = 1;
            const idx = p_count - 1;
            if (idx < 4) {
                has_param[idx] = true;
                params[idx] = if (params[idx] < 1000000) params[idx] * 10 + u32(b - '0') else params[idx];
            }
            i += 1;
        } else if (b == ';') {
            if (p_count == 0) p_count = 1;
            p_count += 1;
            i += 1;
        } else if (b >= 0x20 && b <= 0x3F) {
            i += 1;
        } else {
            break;
        }
    }

    // Incomplete CSI sequence without final byte
    if (i >= n || bytes[i] < 0x40 || bytes[i] > 0x7E) {
        return KeyEvent{
            code: KeyCode.Esc,
            mods: KeyModifiers.none(),
            consumed_bytes: 1,
        };
    }

    const final_byte = bytes[i];
    const consumed = i + 1;

    if (is_private) {
        return KeyEvent{
            code: KeyCode.Unknown,
            mods: KeyModifiers.none(),
            consumed_bytes: consumed,
        };
    }

    // Modifier parameter is params[1] when present (e.g. \x1b[1;5C or \x1b[3;2~)
    let mods = KeyModifiers.none();
    if (p_count >= 2 && has_param[1]) {
        mods = KeyModifiers.fromXtermParam(params[1]);
    }

    if (final_byte == 'A') return KeyEvent{ code: KeyCode.Up, mods: mods, consumed_bytes: consumed };
    if (final_byte == 'B') return KeyEvent{ code: KeyCode.Down, mods: mods, consumed_bytes: consumed };
    if (final_byte == 'C') return KeyEvent{ code: KeyCode.Right, mods: mods, consumed_bytes: consumed };
    if (final_byte == 'D') return KeyEvent{ code: KeyCode.Left, mods: mods, consumed_bytes: consumed };
    if (final_byte == 'H') return KeyEvent{ code: KeyCode.Home, mods: mods, consumed_bytes: consumed };
    if (final_byte == 'F') return KeyEvent{ code: KeyCode.End, mods: mods, consumed_bytes: consumed };
    if (final_byte == 'P') return KeyEvent{ code: KeyCode.F(1), mods: mods, consumed_bytes: consumed };
    if (final_byte == 'Q') return KeyEvent{ code: KeyCode.F(2), mods: mods, consumed_bytes: consumed };
    if (final_byte == 'R') return KeyEvent{ code: KeyCode.F(3), mods: mods, consumed_bytes: consumed };
    if (final_byte == 'S') return KeyEvent{ code: KeyCode.F(4), mods: mods, consumed_bytes: consumed };
    if (final_byte == 'Z') {
        let bt_mods = mods;
        bt_mods.shift = true;
        return KeyEvent{ code: KeyCode.BackTab, mods: bt_mods, consumed_bytes: consumed };
    }

    if (final_byte == '~') {
        const key_num = if (has_param[0]) params[0] else 0;
        const code: KeyCode = switch (key_num) {
            1 | 7 => KeyCode.Home,
            3 => KeyCode.Delete,
            4 | 8 => KeyCode.End,
            5 => KeyCode.PageUp,
            6 => KeyCode.PageDown,
            11 => KeyCode.F(1),
            12 => KeyCode.F(2),
            13 => KeyCode.F(3),
            14 => KeyCode.F(4),
            15 => KeyCode.F(5),
            17 => KeyCode.F(6),
            18 => KeyCode.F(7),
            19 => KeyCode.F(8),
            20 => KeyCode.F(9),
            21 => KeyCode.F(10),
            23 => KeyCode.F(11),
            24 => KeyCode.F(12),
            else => KeyCode.Unknown,
        };
        return KeyEvent{ code: code, mods: mods, consumed_bytes: consumed };
    }

    if (final_byte == 'u' && has_param[0]) {
        const cp = params[0];
        const code: KeyCode = switch (cp) {
            9 => KeyCode.Tab,
            13 => KeyCode.Enter,
            27 => KeyCode.Esc,
            127 => KeyCode.Backspace,
            else => if (cp <= 0x10FFFF) KeyCode.Char(@as(u21, @intCast(cp))) else KeyCode.Unknown,
        };
        return KeyEvent{ code: code, mods: mods, consumed_bytes: consumed };
    }

    return KeyEvent{
        code: KeyCode.Unknown,
        mods: mods,
        consumed_bytes: consumed,
    };
}

fn parseSs3Sequence(bytes: []const u8): KeyEvent {
    // bytes[0] == 0x1B, bytes[1] == 'O'
    if (bytes.len < 3) {
        return KeyEvent{
            code: KeyCode.Char('O'),
            mods: KeyModifiers{ ctrl: false, alt: true, shift: false },
            consumed_bytes: 2,
        };
    }

    const b = bytes[2];
    const code: KeyCode = switch (b) {
        'P' => KeyCode.F(1),
        'Q' => KeyCode.F(2),
        'R' => KeyCode.F(3),
        'S' => KeyCode.F(4),
        'A' => KeyCode.Up,
        'B' => KeyCode.Down,
        'C' => KeyCode.Right,
        'D' => KeyCode.Left,
        'H' => KeyCode.Home,
        'F' => KeyCode.End,
        else => KeyCode.Unknown,
    };
    return KeyEvent{
        code: code,
        mods: KeyModifiers.none(),
        consumed_bytes: 3,
    };
}

fn parseSingleControlOrByte(b: u8, alt: bool): KeyEvent {
    if (b == 0x09) {
        return KeyEvent{
            code: KeyCode.Tab,
            mods: KeyModifiers{ ctrl: false, alt: alt, shift: false },
            consumed_bytes: if (alt) 2 else 1,
        };
    }
    if (b == 0x0D || b == 0x0A) {
        return KeyEvent{
            code: KeyCode.Enter,
            mods: KeyModifiers{ ctrl: false, alt: alt, shift: false },
            consumed_bytes: if (alt) 2 else 1,
        };
    }
    if (b == 0x08 || b == 0x7F) {
        return KeyEvent{
            code: KeyCode.Backspace,
            mods: KeyModifiers{ ctrl: false, alt: alt, shift: false },
            consumed_bytes: if (alt) 2 else 1,
        };
    }
    if (b >= 0x01 && b <= 0x1A) {
        const ch: u21 = @as(u21, 'a') + @as(u21, b - 1);
        return KeyEvent{
            code: KeyCode.Char(ch),
            mods: KeyModifiers{ ctrl: true, alt: alt, shift: false },
            consumed_bytes: if (alt) 2 else 1,
        };
    }
    if (b >= 0x20 && b <= 0x7E) {
        return KeyEvent{
            code: KeyCode.Char(@as(u21, b)),
            mods: KeyModifiers{ ctrl: false, alt: alt, shift: false },
            consumed_bytes: if (alt) 2 else 1,
        };
    }
    return KeyEvent{
        code: KeyCode.Unknown,
        mods: KeyModifiers{ ctrl: false, alt: alt, shift: false },
        consumed_bytes: if (alt) 2 else 1,
    };
}

/// ANSI / VT100 / xterm / SS3 / UTF-8 keyboard input sequence parser.
pub static struct InputParser {
    /// Parses the next key event from the front of `bytes`.
    /// Returns `null` when `bytes` is empty.
    static fn parseNext(bytes: []const u8): KeyEvent? {
        if (bytes.len == 0) return null;

        const first = bytes[0];

        // Escape / CSI / SS3 / Alt+<key>
        if (first == 0x1B) {
            if (bytes.len == 1) {
                return KeyEvent{
                    code: KeyCode.Esc,
                    mods: KeyModifiers.none(),
                    consumed_bytes: 1,
                };
            }

            const second = bytes[1];
            if (second == '[') {
                if (bytes.len == 2) {
                    return KeyEvent{
                        code: KeyCode.Char('['),
                        mods: KeyModifiers{ ctrl: false, alt: true, shift: false },
                        consumed_bytes: 2,
                    };
                }
                return parseCsiSequence(bytes);
            }
            if (second == 'O') {
                return parseSs3Sequence(bytes);
            }
            if (second == 0x1B) {
                return KeyEvent{
                    code: KeyCode.Esc,
                    mods: KeyModifiers.none(),
                    consumed_bytes: 1,
                };
            }
            if (second >= 0x80) {
                if (decoded := decodeUtf8At(bytes[1..])) {
                    return KeyEvent{
                        code: KeyCode.Char(decoded.cp),
                        mods: KeyModifiers{ ctrl: false, alt: true, shift: false },
                        consumed_bytes: 1 + decoded.len,
                    };
                }
                return KeyEvent{
                    code: KeyCode.Unknown,
                    mods: KeyModifiers{ ctrl: false, alt: true, shift: false },
                    consumed_bytes: 2,
                };
            }
            return parseSingleControlOrByte(second, true);
        }

        // Single-byte ASCII or control character
        if (first < 0x80) {
            return parseSingleControlOrByte(first, false);
        }

        // Multi-byte UTF-8 character
        if (decoded := decodeUtf8At(bytes)) {
            return KeyEvent{
                code: KeyCode.Char(decoded.cp),
                mods: KeyModifiers.none(),
                consumed_bytes: decoded.len,
            };
        }

        return KeyEvent{
            code: KeyCode.Unknown,
            mods: KeyModifiers.none(),
            consumed_bytes: 1,
        };
    }
}
