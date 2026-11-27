// @cap/enquirer — Pure CP zero-allocation interactive CLI prompt state machines.
// Clean-room MIT implementation inspired by npm `enquirer` v2.
// Reuses `@cap/picocolors` (`PicoFormatter`) for ANSI prompt symbol and selection highlighting.

import { trim, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { PicoFormatter } from "../../picocolors/src/index.cp";

pub const ENQ_MAX_CHOICES: usize = 16;

/// Interactive yes/no confirmation prompt state machine.
pub struct ConfirmPrompt {
    pub message: string,
    pub initial: bool,

    pub static fn new(message: string, initial: bool): ConfirmPrompt {
        return ConfirmPrompt {
            message: message,
            initial: initial
        };
    }

    /// Parses user confirmation input:
    /// - Empty/whitespace -> `_.initial`
    /// - `y`, `yes`, `true`, `1` (case-insensitive) -> `true`
    /// - `n`, `no`, `false`, `0` (case-insensitive) -> `false`
    /// - Otherwise -> `null`
    pub fn parseAnswer(input: string): bool? {
        let trimmed = trim(input);
        if (trimmed.len == 0) {
            return _.initial;
        }
        if (eqIgnoreCase(trimmed, "y") ||
            eqIgnoreCase(trimmed, "yes") ||
            eqIgnoreCase(trimmed, "true") ||
            eqIgnoreCase(trimmed, "1")) {
            return true;
        }
        if (eqIgnoreCase(trimmed, "n") ||
            eqIgnoreCase(trimmed, "no") ||
            eqIgnoreCase(trimmed, "false") ||
            eqIgnoreCase(trimmed, "0")) {
            return false;
        }
        return null;
    }

    /// Renders the confirm prompt line into `out` using `@cap/picocolors`.
    pub fn render(use_color: bool, out: []u8): usize {
        let fmt = PicoFormatter.new(use_color);
        let pos: usize = 0;
        pos = pos + fmt.cyan("?", out[pos..out.len]);
        pos = writeStr(" ", out, pos);
        pos = pos + fmt.bold(_.message, out[pos..out.len]);
        pos = writeStr(" ", out, pos);
        let hint = "(y/N)";
        if (_.initial) {
            hint = "(Y/n)";
        }
        pos = pos + fmt.dim(hint, out[pos..out.len]);
        return pos;
    }
}

/// A choice entry inside `SelectPrompt`.
pub struct SelectChoice {
    pub label: string,
    pub enabled: bool,
}

/// Zero-heap fixed-16-choice single-select menu state machine.
pub struct SelectPrompt {
    pub message: string,
    pub choices: [16]SelectChoice,
    pub choice_count: usize,
    pub cursor: usize,
    pub submitted: bool,

    pub static fn new(message: string): SelectPrompt {
        let blank = SelectChoice { label: "", enabled: false };
        let arr: [16]SelectChoice = [blank; 16];
        return SelectPrompt {
            message: message,
            choices: arr,
            choice_count: 0,
            cursor: 0,
            submitted: false
        };
    }

    /// Adds a choice to the menu. Returns `false` if the 16-choice buffer is full.
    pub fn addChoice(*_, label: string, enabled: bool): bool {
        if (_.choice_count >= ENQ_MAX_CHOICES) {
            return false;
        }
        let idx = _.choice_count;
        _.choices[idx] = SelectChoice { label: label, enabled: enabled };
        _.choice_count = _.choice_count + 1;
        // Position cursor on the first enabled choice if current cursor is not enabled.
        if (enabled && (idx == 0 || !_.choices[_.cursor].enabled)) {
            _.cursor = idx;
        }
        return true;
    }

    /// Moves cursor downward, skipping disabled choices and wrapping around.
    pub fn moveDown(*_) {
        if (_.choice_count == 0) {
            return;
        }
        let step: usize = 1;
        while (step <= _.choice_count) {
            let next_idx = (_.cursor + step) % _.choice_count;
            if (_.choices[next_idx].enabled) {
                _.cursor = next_idx;
                return;
            }
            step = step + 1;
        }
    }

    /// Moves cursor upward, skipping disabled choices and wrapping around.
    pub fn moveUp(*_) {
        if (_.choice_count == 0) {
            return;
        }
        let step: usize = 1;
        while (step <= _.choice_count) {
            let prev_idx = (_.cursor + _.choice_count - (step % _.choice_count)) % _.choice_count;
            if (_.choices[prev_idx].enabled) {
                _.cursor = prev_idx;
                return;
            }
            step = step + 1;
        }
    }

    /// Submits the currently focused enabled choice, or returns `null` if none is enabled.
    pub fn submit(*_): string? {
        if (_.choice_count == 0 || _.cursor >= _.choice_count) {
            return null;
        }
        if (!_.choices[_.cursor].enabled) {
            return null;
        }
        _.submitted = true;
        return _.choices[_.cursor].label;
    }

    /// Renders the select prompt and choices into `out`, applying `@cap/picocolors` when `use_color` is true.
    pub fn render(use_color: bool, out: []u8): usize {
        let fmt = PicoFormatter.new(use_color);
        let pos: usize = 0;
        if (_.submitted) {
            pos = pos + fmt.green("v", out[pos..out.len]);
        } else {
            pos = pos + fmt.cyan("?", out[pos..out.len]);
        }
        pos = writeStr(" ", out, pos);
        pos = pos + fmt.bold(_.message, out[pos..out.len]);
        pos = writeStr("\n", out, pos);

        let i: usize = 0;
        while (i < _.choice_count) {
            let item = _.choices[i];
            if (i == _.cursor && item.enabled) {
                pos = pos + fmt.cyan("> ", out[pos..out.len]);
                pos = pos + fmt.cyan(item.label, out[pos..out.len]);
            } else if (!item.enabled) {
                pos = writeStr("  ", out, pos);
                pos = pos + fmt.dim(item.label, out[pos..out.len]);
            } else {
                pos = writeStr("  ", out, pos);
                pos = writeStr(item.label, out, pos);
            }
            pos = writeStr("\n", out, pos);
            i = i + 1;
        }
        return pos;
    }
}

/// Zero-heap fixed-16-choice multi-select menu state machine.
pub struct MultiSelectPrompt {
    pub message: string,
    pub choices: [16]string,
    pub selected: [16]bool,
    pub choice_count: usize,

    pub static fn new(message: string): MultiSelectPrompt {
        let c_arr: [16]string = [""; 16];
        let s_arr: [16]bool = [false; 16];
        return MultiSelectPrompt {
            message: message,
            choices: c_arr,
            selected: s_arr,
            choice_count: 0
        };
    }

    /// Adds a selectable choice. Returns `false` if the 16-choice table is full.
    pub fn addChoice(*_, label: string): bool {
        if (_.choice_count >= ENQ_MAX_CHOICES) {
            return false;
        }
        _.choices[_.choice_count] = label;
        _.selected[_.choice_count] = false;
        _.choice_count = _.choice_count + 1;
        return true;
    }

    /// Toggles selection state for `idx`. Returns `false` if `idx` is out of bounds.
    pub fn toggle(*_, idx: usize): bool {
        if (idx >= _.choice_count) {
            return false;
        }
        _.selected[idx] = !_.selected[idx];
        return true;
    }

    /// Returns the total number of currently selected choices.
    pub fn selectedCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.choice_count) {
            if (_.selected[i]) {
                count = count + 1;
            }
            i = i + 1;
        }
        return count;
    }

    /// Returns `true` if `idx` is valid and currently selected.
    pub fn isSelected(idx: usize): bool {
        if (idx >= _.choice_count) {
            return false;
        }
        return _.selected[idx];
    }

    /// Renders the multi-select checklist into `out` using `@cap/picocolors`.
    pub fn render(use_color: bool, out: []u8): usize {
        let fmt = PicoFormatter.new(use_color);
        let pos: usize = 0;
        pos = pos + fmt.cyan("?", out[pos..out.len]);
        pos = writeStr(" ", out, pos);
        pos = pos + fmt.bold(_.message, out[pos..out.len]);
        pos = writeStr("\n", out, pos);

        let i: usize = 0;
        while (i < _.choice_count) {
            if (_.selected[i]) {
                pos = pos + fmt.green("[x] ", out[pos..out.len]);
            } else {
                pos = pos + fmt.dim("[ ] ", out[pos..out.len]);
            }
            pos = writeStr(_.choices[i], out, pos);
            pos = writeStr("\n", out, pos);
            i = i + 1;
        }
        return pos;
    }
}
