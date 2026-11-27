// @cap/yn — Zero-allocation yes/no boolean string parser in pure CP
// Clean-room MIT implementation of npm `yn` (#978, 56.8M/wk).

import { trim, eqIgnoreCase } from "cap:ascii";

/// Parses standard yes/no boolean strings (`y`, `yes`, `true`, `1`, `on` -> `true`;
/// `n`, `no`, `false`, `0`, `off` -> `false`; unrecognized -> `null`).
pub fn yn(input: string): bool? {
    const s = trim(input);
    if (s.len == 0 || s.len > 10) {
        return null;
    }
    if (eqIgnoreCase(s, "y") || eqIgnoreCase(s, "yes") || eqIgnoreCase(s, "true") ||
        eqIgnoreCase(s, "1") || eqIgnoreCase(s, "on")) {
        return true;
    }
    if (eqIgnoreCase(s, "n") || eqIgnoreCase(s, "no") || eqIgnoreCase(s, "false") ||
        eqIgnoreCase(s, "0") || eqIgnoreCase(s, "off")) {
        return false;
    }
    return null;
}

/// Parses yes/no strings with optional `lenient` typo tolerance (e.g. `yse`, `yess`, `noo`, `truu`, `flase`).
pub fn ynLenient(input: string): bool? {
    const strict = yn(input);
    if (strict != null) {
        return strict;
    }
    const s = trim(input);
    if (eqIgnoreCase(s, "yse") || eqIgnoreCase(s, "yess") || eqIgnoreCase(s, "ye") ||
        eqIgnoreCase(s, "ys") || eqIgnoreCase(s, "es") || eqIgnoreCase(s, "truu") || eqIgnoreCase(s, "ture")) {
        return true;
    }
    if (eqIgnoreCase(s, "noo") || eqIgnoreCase(s, "nop") || eqIgnoreCase(s, "nope") ||
        eqIgnoreCase(s, "flase") || eqIgnoreCase(s, "fasle")) {
        return false;
    }
    return null;
}

/// Parses `input` with a `default_val` fallback when unrecognized.
pub fn ynDefault(input: string, default_val: bool): bool {
    return yn(input) ?? default_val;
}

export {
    yn,
    ynLenient,
    ynDefault,
};
