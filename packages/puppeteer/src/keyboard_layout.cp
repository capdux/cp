// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/common/USKeyboardLayout.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/keyboard_layout.cp — US Keyboard Layout Lookup Table
// O(1) ASCII fast-path + static zero-heap-allocation KeyDefinition table
// ============================================================================

const std = @import("std");

pub struct KeyDefinition {
    name: []const u8 = "",
    key_code: u16 = 0,
    shift_key_code: u16 = 0,
    key: []const u8 = "",
    shift_key: []const u8 = "",
    code: []const u8 = "",
    text: []const u8 = "",
    shift_text: []const u8 = "",
    location: u8 = 0,
}

const NAMED_KEYS: [110]KeyDefinition = .{
    KeyDefinition{ name: "Backspace", key_code: 8, code: "Backspace", key: "Backspace" },
    KeyDefinition{ name: "Tab", key_code: 9, code: "Tab", key: "Tab" },
    KeyDefinition{ name: "Enter", key_code: 13, code: "Enter", key: "Enter", text: "\r" },
    KeyDefinition{ name: "NumpadEnter", key_code: 13, code: "NumpadEnter", key: "Enter", text: "\r", location: 3 },
    KeyDefinition{ name: "ShiftLeft", key_code: 16, code: "ShiftLeft", key: "Shift", location: 1 },
    KeyDefinition{ name: "ShiftRight", key_code: 16, code: "ShiftRight", key: "Shift", location: 2 },
    KeyDefinition{ name: "Shift", key_code: 16, code: "ShiftLeft", key: "Shift", location: 1 },
    KeyDefinition{ name: "ControlLeft", key_code: 17, code: "ControlLeft", key: "Control", location: 1 },
    KeyDefinition{ name: "ControlRight", key_code: 17, code: "ControlRight", key: "Control", location: 2 },
    KeyDefinition{ name: "Control", key_code: 17, code: "ControlLeft", key: "Control", location: 1 },
    KeyDefinition{ name: "AltLeft", key_code: 18, code: "AltLeft", key: "Alt", location: 1 },
    KeyDefinition{ name: "AltRight", key_code: 18, code: "AltRight", key: "Alt", location: 2 },
    KeyDefinition{ name: "Alt", key_code: 18, code: "AltLeft", key: "Alt", location: 1 },
    KeyDefinition{ name: "MetaLeft", key_code: 91, code: "MetaLeft", key: "Meta", location: 1 },
    KeyDefinition{ name: "MetaRight", key_code: 92, code: "MetaRight", key: "Meta", location: 2 },
    KeyDefinition{ name: "Meta", key_code: 91, code: "MetaLeft", key: "Meta", location: 1 },
    KeyDefinition{ name: "Pause", key_code: 19, code: "Pause", key: "Pause" },
    KeyDefinition{ name: "CapsLock", key_code: 20, code: "CapsLock", key: "CapsLock" },
    KeyDefinition{ name: "Escape", key_code: 27, code: "Escape", key: "Escape" },
    KeyDefinition{ name: "Space", key_code: 32, code: "Space", key: " ", text: " " },
    KeyDefinition{ name: "PageUp", key_code: 33, code: "PageUp", key: "PageUp" },
    KeyDefinition{ name: "PageDown", key_code: 34, code: "PageDown", key: "PageDown" },
    KeyDefinition{ name: "End", key_code: 35, code: "End", key: "End" },
    KeyDefinition{ name: "Home", key_code: 36, code: "Home", key: "Home" },
    KeyDefinition{ name: "ArrowLeft", key_code: 37, code: "ArrowLeft", key: "ArrowLeft" },
    KeyDefinition{ name: "ArrowUp", key_code: 38, code: "ArrowUp", key: "ArrowUp" },
    KeyDefinition{ name: "ArrowRight", key_code: 39, code: "ArrowRight", key: "ArrowRight" },
    KeyDefinition{ name: "ArrowDown", key_code: 40, code: "ArrowDown", key: "ArrowDown" },
    KeyDefinition{ name: "PrintScreen", key_code: 44, code: "PrintScreen", key: "PrintScreen" },
    KeyDefinition{ name: "Insert", key_code: 45, code: "Insert", key: "Insert" },
    KeyDefinition{ name: "Delete", key_code: 46, code: "Delete", key: "Delete" },
    KeyDefinition{ name: "ContextMenu", key_code: 93, code: "ContextMenu", key: "ContextMenu" },
    KeyDefinition{ name: "NumLock", key_code: 144, code: "NumLock", key: "NumLock" },
    KeyDefinition{ name: "ScrollLock", key_code: 145, code: "ScrollLock", key: "ScrollLock" },
    KeyDefinition{ name: "AltGraph", key_code: 225, code: "AltGraph", key: "AltGraph" },
    KeyDefinition{ name: "Digit0", key_code: 48, code: "Digit0", key: "0", shift_key: ")" },
    KeyDefinition{ name: "Digit1", key_code: 49, code: "Digit1", key: "1", shift_key: "!" },
    KeyDefinition{ name: "Digit2", key_code: 50, code: "Digit2", key: "2", shift_key: "@" },
    KeyDefinition{ name: "Digit3", key_code: 51, code: "Digit3", key: "3", shift_key: "#" },
    KeyDefinition{ name: "Digit4", key_code: 52, code: "Digit4", key: "4", shift_key: "$" },
    KeyDefinition{ name: "Digit5", key_code: 53, code: "Digit5", key: "5", shift_key: "%" },
    KeyDefinition{ name: "Digit6", key_code: 54, code: "Digit6", key: "6", shift_key: "^" },
    KeyDefinition{ name: "Digit7", key_code: 55, code: "Digit7", key: "7", shift_key: "&" },
    KeyDefinition{ name: "Digit8", key_code: 56, code: "Digit8", key: "8", shift_key: "*" },
    KeyDefinition{ name: "Digit9", key_code: 57, code: "Digit9", key: "9", shift_key: "(" },
    KeyDefinition{ name: "KeyA", key_code: 65, code: "KeyA", key: "a", shift_key: "A" },
    KeyDefinition{ name: "KeyB", key_code: 66, code: "KeyB", key: "b", shift_key: "B" },
    KeyDefinition{ name: "KeyC", key_code: 67, code: "KeyC", key: "c", shift_key: "C" },
    KeyDefinition{ name: "KeyD", key_code: 68, code: "KeyD", key: "d", shift_key: "D" },
    KeyDefinition{ name: "KeyE", key_code: 69, code: "KeyE", key: "e", shift_key: "E" },
    KeyDefinition{ name: "KeyF", key_code: 70, code: "KeyF", key: "f", shift_key: "F" },
    KeyDefinition{ name: "KeyG", key_code: 71, code: "KeyG", key: "g", shift_key: "G" },
    KeyDefinition{ name: "KeyH", key_code: 72, code: "KeyH", key: "h", shift_key: "H" },
    KeyDefinition{ name: "KeyI", key_code: 73, code: "KeyI", key: "i", shift_key: "I" },
    KeyDefinition{ name: "KeyJ", key_code: 74, code: "KeyJ", key: "j", shift_key: "J" },
    KeyDefinition{ name: "KeyK", key_code: 75, code: "KeyK", key: "k", shift_key: "K" },
    KeyDefinition{ name: "KeyL", key_code: 76, code: "KeyL", key: "l", shift_key: "L" },
    KeyDefinition{ name: "KeyM", key_code: 77, code: "KeyM", key: "m", shift_key: "M" },
    KeyDefinition{ name: "KeyN", key_code: 78, code: "KeyN", key: "n", shift_key: "N" },
    KeyDefinition{ name: "KeyO", key_code: 79, code: "KeyO", key: "o", shift_key: "O" },
    KeyDefinition{ name: "KeyP", key_code: 80, code: "KeyP", key: "p", shift_key: "P" },
    KeyDefinition{ name: "KeyQ", key_code: 81, code: "KeyQ", key: "q", shift_key: "Q" },
    KeyDefinition{ name: "KeyR", key_code: 82, code: "KeyR", key: "r", shift_key: "R" },
    KeyDefinition{ name: "KeyS", key_code: 83, code: "KeyS", key: "s", shift_key: "S" },
    KeyDefinition{ name: "KeyT", key_code: 84, code: "KeyT", key: "t", shift_key: "T" },
    KeyDefinition{ name: "KeyU", key_code: 85, code: "KeyU", key: "u", shift_key: "U" },
    KeyDefinition{ name: "KeyV", key_code: 86, code: "KeyV", key: "v", shift_key: "V" },
    KeyDefinition{ name: "KeyW", key_code: 87, code: "KeyW", key: "w", shift_key: "W" },
    KeyDefinition{ name: "KeyX", key_code: 88, code: "KeyX", key: "x", shift_key: "X" },
    KeyDefinition{ name: "KeyY", key_code: 89, code: "KeyY", key: "y", shift_key: "Y" },
    KeyDefinition{ name: "KeyZ", key_code: 90, code: "KeyZ", key: "z", shift_key: "Z" },
    KeyDefinition{ name: "F1", key_code: 112, code: "F1", key: "F1" },
    KeyDefinition{ name: "F2", key_code: 113, code: "F2", key: "F2" },
    KeyDefinition{ name: "F3", key_code: 114, code: "F3", key: "F3" },
    KeyDefinition{ name: "F4", key_code: 115, code: "F4", key: "F4" },
    KeyDefinition{ name: "F5", key_code: 116, code: "F5", key: "F5" },
    KeyDefinition{ name: "F6", key_code: 117, code: "F6", key: "F6" },
    KeyDefinition{ name: "F7", key_code: 118, code: "F7", key: "F7" },
    KeyDefinition{ name: "F8", key_code: 119, code: "F8", key: "F8" },
    KeyDefinition{ name: "F9", key_code: 120, code: "F9", key: "F9" },
    KeyDefinition{ name: "F10", key_code: 121, code: "F10", key: "F10" },
    KeyDefinition{ name: "F11", key_code: 122, code: "F11", key: "F11" },
    KeyDefinition{ name: "F12", key_code: 123, code: "F12", key: "F12" },
    KeyDefinition{ name: "Numpad0", key_code: 45, shift_key_code: 96, code: "Numpad0", key: "Insert", shift_key: "0", location: 3 },
    KeyDefinition{ name: "Numpad1", key_code: 35, shift_key_code: 97, code: "Numpad1", key: "End", shift_key: "1", location: 3 },
    KeyDefinition{ name: "Numpad2", key_code: 40, shift_key_code: 98, code: "Numpad2", key: "ArrowDown", shift_key: "2", location: 3 },
    KeyDefinition{ name: "Numpad3", key_code: 34, shift_key_code: 99, code: "Numpad3", key: "PageDown", shift_key: "3", location: 3 },
    KeyDefinition{ name: "Numpad4", key_code: 37, shift_key_code: 100, code: "Numpad4", key: "ArrowLeft", shift_key: "4", location: 3 },
    KeyDefinition{ name: "Numpad5", key_code: 12, shift_key_code: 101, code: "Numpad5", key: "Clear", shift_key: "5", location: 3 },
    KeyDefinition{ name: "Numpad6", key_code: 39, shift_key_code: 102, code: "Numpad6", key: "ArrowRight", shift_key: "6", location: 3 },
    KeyDefinition{ name: "Numpad7", key_code: 36, shift_key_code: 103, code: "Numpad7", key: "Home", shift_key: "7", location: 3 },
    KeyDefinition{ name: "Numpad8", key_code: 38, shift_key_code: 104, code: "Numpad8", key: "ArrowUp", shift_key: "8", location: 3 },
    KeyDefinition{ name: "Numpad9", key_code: 33, shift_key_code: 105, code: "Numpad9", key: "PageUp", shift_key: "9", location: 3 },
    KeyDefinition{ name: "NumpadMultiply", key_code: 106, code: "NumpadMultiply", key: "*", location: 3 },
    KeyDefinition{ name: "NumpadAdd", key_code: 107, code: "NumpadAdd", key: "+", location: 3 },
    KeyDefinition{ name: "NumpadSubtract", key_code: 109, code: "NumpadSubtract", key: "-", location: 3 },
    KeyDefinition{ name: "NumpadDivide", key_code: 111, code: "NumpadDivide", key: "/", location: 3 },
    KeyDefinition{ name: "NumpadDecimal", key_code: 46, shift_key_code: 110, code: "NumpadDecimal", key: "\x00", shift_key: ".", location: 3 },
    KeyDefinition{ name: "NumpadEqual", key_code: 187, code: "NumpadEqual", key: "=", location: 3 },
    KeyDefinition{ name: "Semicolon", key_code: 186, code: "Semicolon", key: ";", shift_key: ":" },
    KeyDefinition{ name: "Equal", key_code: 187, code: "Equal", key: "=", shift_key: "+" },
    KeyDefinition{ name: "Comma", key_code: 188, code: "Comma", key: ",", shift_key: "<" },
    KeyDefinition{ name: "Minus", key_code: 189, code: "Minus", key: "-", shift_key: "_" },
    KeyDefinition{ name: "Period", key_code: 190, code: "Period", key: ".", shift_key: ">" },
    KeyDefinition{ name: "Slash", key_code: 191, code: "Slash", key: "/", shift_key: "?" },
    KeyDefinition{ name: "Backquote", key_code: 192, code: "Backquote", key: "`", shift_key: "~" },
    KeyDefinition{ name: "BracketLeft", key_code: 219, code: "BracketLeft", key: "[", shift_key: "{" },
    KeyDefinition{ name: "Backslash", key_code: 220, code: "Backslash", key: "\\", shift_key: "|" },
    KeyDefinition{ name: "BracketRight", key_code: 221, code: "BracketRight", key: "]", shift_key: "}" },
    KeyDefinition{ name: "Quote", key_code: 222, code: "Quote", key: "'", shift_key: "\"" },
};

const LOWER_CODES: [26][]const u8 = .{
    "KeyA", "KeyB", "KeyC", "KeyD", "KeyE", "KeyF", "KeyG", "KeyH", "KeyI", "KeyJ",
    "KeyK", "KeyL", "KeyM", "KeyN", "KeyO", "KeyP", "KeyQ", "KeyR", "KeyS", "KeyT",
    "KeyU", "KeyV", "KeyW", "KeyX", "KeyY", "KeyZ",
};

const LOWER_KEYS: [26][]const u8 = .{
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
    "k", "l", "m", "n", "o", "p", "q", "r", "s", "t",
    "u", "v", "w", "x", "y", "z",
};

const UPPER_KEYS: [26][]const u8 = .{
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
    "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
    "U", "V", "W", "X", "Y", "Z",
};

const DIGIT_CODES: [10][]const u8 = .{
    "Digit0", "Digit1", "Digit2", "Digit3", "Digit4",
    "Digit5", "Digit6", "Digit7", "Digit8", "Digit9",
};

const DIGIT_KEYS: [10][]const u8 = .{
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
};

fn lookupAsciiChar(ch: u8): KeyDefinition? {
    if (ch >= 'a' && ch <= 'z') {
        const idx: usize = @as(usize, ch - 'a');
        return KeyDefinition{
            name: LOWER_KEYS[idx],
            key_code: @as(u16, ch - 32),
            key: LOWER_KEYS[idx],
            code: LOWER_CODES[idx],
            text: LOWER_KEYS[idx],
        };
    }
    if (ch >= 'A' && ch <= 'Z') {
        const idx: usize = @as(usize, ch - 'A');
        return KeyDefinition{
            name: UPPER_KEYS[idx],
            key_code: @as(u16, ch),
            key: UPPER_KEYS[idx],
            code: LOWER_CODES[idx],
            text: UPPER_KEYS[idx],
        };
    }
    if (ch >= '0' && ch <= '9') {
        const idx: usize = @as(usize, ch - '0');
        return KeyDefinition{
            name: DIGIT_KEYS[idx],
            key_code: @as(u16, ch),
            key: DIGIT_KEYS[idx],
            code: DIGIT_CODES[idx],
            text: DIGIT_KEYS[idx],
        };
    }
    switch (ch) {
        ' ' => return KeyDefinition{ name: " ", key_code: 32, key: " ", code: "Space", text: " " },
        '\r', '\n' => return KeyDefinition{ name: "\r", key_code: 13, key: "Enter", code: "Enter", text: "\r" },
        ';' => return KeyDefinition{ name: ";", key_code: 186, key: ";", code: "Semicolon", text: ";" },
        ':' => return KeyDefinition{ name: ":", key_code: 186, key: ":", code: "Semicolon", text: ":" },
        '=' => return KeyDefinition{ name: "=", key_code: 187, key: "=", code: "Equal", text: "=" },
        '+' => return KeyDefinition{ name: "+", key_code: 107, key: "+", code: "NumpadAdd", text: "+", location: 3 },
        ',' => return KeyDefinition{ name: ",", key_code: 188, key: ",", code: "Comma", text: "," },
        '<' => return KeyDefinition{ name: "<", key_code: 188, key: "<", code: "Comma", text: "<" },
        '-' => return KeyDefinition{ name: "-", key_code: 109, key: "-", code: "NumpadSubtract", text: "-", location: 3 },
        '_' => return KeyDefinition{ name: "_", key_code: 189, key: "_", code: "Minus", text: "_" },
        '.' => return KeyDefinition{ name: ".", key_code: 190, key: ".", code: "Period", text: "." },
        '>' => return KeyDefinition{ name: ">", key_code: 190, key: ">", code: "Period", text: ">" },
        '/' => return KeyDefinition{ name: "/", key_code: 111, key: "/", code: "NumpadDivide", text: "/", location: 3 },
        '?' => return KeyDefinition{ name: "?", key_code: 191, key: "?", code: "Slash", text: "?" },
        '`' => return KeyDefinition{ name: "`", key_code: 192, key: "`", code: "Backquote", text: "`" },
        '~' => return KeyDefinition{ name: "~", key_code: 192, key: "~", code: "Backquote", text: "~" },
        '[' => return KeyDefinition{ name: "[", key_code: 219, key: "[", code: "BracketLeft", text: "[" },
        '{' => return KeyDefinition{ name: "{", key_code: 219, key: "{", code: "BracketLeft", text: "{" },
        '\\' => return KeyDefinition{ name: "\\", key_code: 220, key: "\\", code: "Backslash", text: "\\" },
        '|' => return KeyDefinition{ name: "|", key_code: 220, key: "|", code: "Backslash", text: "|" },
        ']' => return KeyDefinition{ name: "]", key_code: 221, key: "]", code: "BracketRight", text: "]" },
        '}' => return KeyDefinition{ name: "}", key_code: 221, key: "}", code: "BracketRight", text: "}" },
        '\'' => return KeyDefinition{ name: "'", key_code: 222, key: "'", code: "Quote", text: "'" },
        '"' => return KeyDefinition{ name: "\"", key_code: 222, key: "\"", code: "Quote", text: "\"" },
        '!' => return KeyDefinition{ name: "!", key_code: 49, key: "!", code: "Digit1", text: "!" },
        '@' => return KeyDefinition{ name: "@", key_code: 50, key: "@", code: "Digit2", text: "@" },
        '#' => return KeyDefinition{ name: "#", key_code: 51, key: "#", code: "Digit3", text: "#" },
        '$' => return KeyDefinition{ name: "$", key_code: 52, key: "$", code: "Digit4", text: "$" },
        '%' => return KeyDefinition{ name: "%", key_code: 53, key: "%", code: "Digit5", text: "%" },
        '^' => return KeyDefinition{ name: "^", key_code: 54, key: "^", code: "Digit6", text: "^" },
        '&' => return KeyDefinition{ name: "&", key_code: 55, key: "&", code: "Digit7", text: "&" },
        '*' => return KeyDefinition{ name: "*", key_code: 106, key: "*", code: "NumpadMultiply", text: "*", location: 3 },
        '(' => return KeyDefinition{ name: "(", key_code: 57, key: "(", code: "Digit9", text: "(" },
        ')' => return KeyDefinition{ name: ")", key_code: 48, key: ")", code: "Digit0", text: ")" },
        else => return null,
    }
}

pub struct USKeyboardLayout {
    pub static fn count(): usize {
        return NAMED_KEYS.len;
    }

    pub static fn get(key_input: []const u8): KeyDefinition? {
        if (key_input.len == 0) return null;
        if (key_input.len == 1) {
            return lookupAsciiChar(key_input[0]);
        }
        for (item of NAMED_KEYS) {
            if (item.name.len == key_input.len && item.name[0] == key_input[0]) {
                if (std.mem.eql(u8, item.name, key_input)) {
                    return item;
                }
            }
        }
        return null;
    }

    /// Resolve a key input taking Shift modifier state into account.
    pub static fn resolve(key_input: []const u8, shift_down: bool): KeyDefinition {
        if (def := USKeyboardLayout.get(key_input)) {
            let resolved = def;
            if (shift_down && def.shift_key.len > 0) {
                resolved.key = def.shift_key;
                if (def.shift_text.len > 0) {
                    resolved.text = def.shift_text;
                } else if (def.shift_key.len == 1) {
                    resolved.text = def.shift_key;
                }
                if (def.shift_key_code != 0) {
                    resolved.key_code = def.shift_key_code;
                }
            } else if (resolved.text.len == 0 && resolved.key.len == 1) {
                resolved.text = resolved.key;
            }
            return resolved;
        }
        return KeyDefinition{
            name: key_input,
            key: key_input,
            code: key_input,
            text: if (key_input.len == 1) key_input else "",
        };
    }
}
