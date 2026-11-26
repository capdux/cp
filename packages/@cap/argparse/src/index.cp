// packages/@cap/argparse/src/index.cp
// Clean-room zero-allocation Python-style CLI argument parser for CP.
// Parity with npm:argparse v2.

pub const ARGP_MAX_DEFS: usize = 32;
pub const ARGP_MAX_POS: usize = 32;

pub const ARGP_ERR_NONE: i32 = 0;
pub const ARGP_ERR_UNKNOWN_FLAG: i32 = 1;
pub const ARGP_ERR_MISSING_VALUE: i32 = 2;
pub const ARGP_ERR_MISSING_REQUIRED: i32 = 3;
pub const ARGP_ERR_TOO_MANY_POSITIONALS: i32 = 4;

pub enum ArgAction {
    Store,
    StoreTrue,
    StoreFalse,
    Count,
}

pub struct ArgDef {
    short_flag: u8,
    long_name: string,
    dest: string,
    action: ArgAction,
    required: bool,
    default_val: string,
    is_positional: bool,
}

pub struct ArgSlot {
    dest: string,
    action: ArgAction,
    has_str: bool,
    str_val: string,
    bool_val: bool,
    count_val: u32,
    seen: bool,
}

fn argpStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn argpStripDashes(s: string): string {
    if (s.len >= 2 && s[0] == '-' && s[1] == '-') {
        return s[2..s.len];
    }
    if (s.len >= 1 && s[0] == '-') {
        return s[1..s.len];
    }
    return s;
}

fn argpFindEq(s: string): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '=') return i;
    }
    return null;
}

fn argpResolveDest(def: ArgDef): string {
    if (def.dest.len > 0) return def.dest;
    return argpStripDashes(def.long_name);
}

pub struct ParseResult {
    ok: bool,
    err_code: i32,
    slot_count: usize,
    slots: [32]ArgSlot,
    pos_count: usize,
    positionals: [32]string,

    fn getString(dest: string): string? {
        let i: usize = 0;
        while (i < _.slot_count) : (i += 1) {
            if (argpStrEq(_.slots[i].dest, dest)) {
                if (_.slots[i].has_str) {
                    return _.slots[i].str_val;
                }
                return null;
            }
        }
        return null;
    }

    fn getBool(dest: string): bool {
        let i: usize = 0;
        while (i < _.slot_count) : (i += 1) {
            if (argpStrEq(_.slots[i].dest, dest)) {
                return _.slots[i].bool_val;
            }
        }
        return false;
    }

    fn getCount(dest: string): u32 {
        let i: usize = 0;
        while (i < _.slot_count) : (i += 1) {
            if (argpStrEq(_.slots[i].dest, dest)) {
                return _.slots[i].count_val;
            }
        }
        return 0;
    }

    fn positionalCount(): usize {
        return _.pos_count;
    }

    fn positionalAt(idx: usize): string? {
        if (idx < _.pos_count) {
            return _.positionals[idx];
        }
        return null;
    }
}

fn argpFindLong(defs: []const ArgDef, name: string): usize? {
    let i: usize = 0;
    while (i < defs.len) : (i += 1) {
        if (defs[i].is_positional) continue;
        const norm = argpStripDashes(defs[i].long_name);
        if (norm.len > 0 && argpStrEq(norm, name)) {
            return i;
        }
    }
    return null;
}

fn argpFindShort(defs: []const ArgDef, ch: u8): usize? {
    if (ch == 0) return null;
    let i: usize = 0;
    while (i < defs.len) : (i += 1) {
        if (defs[i].is_positional) continue;
        if (defs[i].short_flag == ch) {
            return i;
        }
    }
    return null;
}

pub struct ArgParser {
    def_count: usize,
    defs: [32]ArgDef,

    static fn init(): ArgParser {
        let empty_defs: [32]ArgDef = undefined;
        let p = ArgParser{
            def_count: 0,
            defs: empty_defs,
        };
        return p;
    }

    fn addArgument(*_, def: ArgDef): bool {
        if (_.def_count >= ARGP_MAX_DEFS) {
            return false;
        }
        _.defs[_.def_count] = def;
        _.def_count += 1;
        return true;
    }

    fn parseArgs(argv: []const string): ParseResult {
        let slots: [32]ArgSlot = undefined;
        let pos_arr: [32]string = undefined;
        let d: usize = 0;
        while (d < _.def_count) : (d += 1) {
            const def = _.defs[d];
            const dest_name = argpResolveDest(def);
            let has_str = false;
            let str_val: string = "";
            let bool_val = false;
            if (def.action == .Store && def.default_val.len > 0) {
                has_str = true;
                str_val = def.default_val;
            } else if (def.action == .StoreFalse) {
                bool_val = true;
            }
            slots[d] = ArgSlot{
                dest: dest_name,
                action: def.action,
                has_str: has_str,
                str_val: str_val,
                bool_val: bool_val,
                count_val: 0,
                seen: false,
            };
        }

        let res = ParseResult{
            ok: true,
            err_code: ARGP_ERR_NONE,
            slot_count: _.def_count,
            slots: slots,
            pos_count: 0,
            positionals: pos_arr,
        };

        const defs_slice = _.defs[0.._.def_count];
        let after_dd = false;
        let next_pos_def: usize = 0;
        let i: usize = 0;

        while (i < argv.len) {
            const tok = argv[i];

            if (!after_dd && argpStrEq(tok, "--")) {
                after_dd = true;
                i += 1;
                continue;
            }

            if (!after_dd && tok.len > 2 && tok[0] == '-' && tok[1] == '-') {
                const body = tok[2..tok.len];
                let flag_name = body;
                let has_inline_val = false;
                let inline_val: string = "";
                if (eq_pos := argpFindEq(body)) {
                    flag_name = body[0..eq_pos];
                    has_inline_val = true;
                    inline_val = body[eq_pos + 1..body.len];
                }

                if (idx := argpFindLong(defs_slice, flag_name)) {
                    const act = _.defs[idx].action;
                    res.slots[idx].seen = true;
                    if (act == .Store) {
                        if (has_inline_val) {
                            res.slots[idx].has_str = true;
                            res.slots[idx].str_val = inline_val;
                        } else {
                            if (i + 1 >= argv.len) {
                                res.ok = false;
                                res.err_code = ARGP_ERR_MISSING_VALUE;
                                return res;
                            }
                            i += 1;
                            res.slots[idx].has_str = true;
                            res.slots[idx].str_val = argv[i];
                        }
                    } else if (act == .StoreTrue) {
                        res.slots[idx].bool_val = true;
                    } else if (act == .StoreFalse) {
                        res.slots[idx].bool_val = false;
                    } else if (act == .Count) {
                        res.slots[idx].count_val += 1;
                    }
                } else {
                    res.ok = false;
                    res.err_code = ARGP_ERR_UNKNOWN_FLAG;
                    return res;
                }
                i += 1;
                continue;
            }

            if (!after_dd && tok.len >= 2 && tok[0] == '-' && tok[1] != '-') {
                let j: usize = 1;
                while (j < tok.len) {
                    const ch = tok[j];
                    if (idx := argpFindShort(defs_slice, ch)) {
                        const act = _.defs[idx].action;
                        res.slots[idx].seen = true;
                        if (act == .Store) {
                            if (j + 1 < tok.len) {
                                res.slots[idx].has_str = true;
                                res.slots[idx].str_val = tok[j + 1..tok.len];
                                break;
                            } else {
                                if (i + 1 >= argv.len) {
                                    res.ok = false;
                                    res.err_code = ARGP_ERR_MISSING_VALUE;
                                    return res;
                                }
                                i += 1;
                                res.slots[idx].has_str = true;
                                res.slots[idx].str_val = argv[i];
                                break;
                            }
                        } else if (act == .StoreTrue) {
                            res.slots[idx].bool_val = true;
                        } else if (act == .StoreFalse) {
                            res.slots[idx].bool_val = false;
                        } else if (act == .Count) {
                            res.slots[idx].count_val += 1;
                        }
                    } else {
                        res.ok = false;
                        res.err_code = ARGP_ERR_UNKNOWN_FLAG;
                        return res;
                    }
                    j += 1;
                }
                i += 1;
                continue;
            }

            // Positional argument
            if (res.pos_count >= ARGP_MAX_POS) {
                res.ok = false;
                res.err_code = ARGP_ERR_TOO_MANY_POSITIONALS;
                return res;
            }
            res.positionals[res.pos_count] = tok;
            res.pos_count += 1;

            // Bind to next positional ArgDef if present
            while (next_pos_def < _.def_count && !_.defs[next_pos_def].is_positional) {
                next_pos_def += 1;
            }
            if (next_pos_def < _.def_count) {
                res.slots[next_pos_def].seen = true;
                res.slots[next_pos_def].has_str = true;
                res.slots[next_pos_def].str_val = tok;
                next_pos_def += 1;
            }

            i += 1;
        }

        // Validate required arguments
        let r: usize = 0;
        while (r < _.def_count) : (r += 1) {
            if (_.defs[r].required && !res.slots[r].seen) {
                res.ok = false;
                res.err_code = ARGP_ERR_MISSING_REQUIRED;
                return res;
            }
        }

        return res;
    }
}
