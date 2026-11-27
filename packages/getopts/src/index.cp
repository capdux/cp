// @cap/getopts — Zero-allocation POSIX/GNU command-line option parser
// Inspired by crates.io:getopts (https://github.com/rust-lang/getopts)

pub const GOPT_MAX_OPTS: usize = 32;
pub const GOPT_MAX_VALUES: usize = 64;
pub const GOPT_MAX_FREE: usize = 64;

pub const GOPT_OK: i32 = 0;
pub const GOPT_ERR_ARGUMENT_MISSING: i32 = 1;
pub const GOPT_ERR_UNRECOGNIZED_OPTION: i32 = 2;
pub const GOPT_ERR_OPTION_MISSING: i32 = 3;
pub const GOPT_ERR_OPTION_DUPLICATED: i32 = 4;
pub const GOPT_ERR_UNEXPECTED_ARGUMENT: i32 = 5;

pub enum HasArg {
    No,
    Yes,
    Maybe,
}

pub enum Occur {
    Optional,
    Req,
    Multi,
}

pub struct OptDef {
    short_name: string,
    long_name: string,
    desc: string,
    has_arg: HasArg,
    occur: Occur,

    static fn empty(): OptDef {
        return OptDef{
            short_name: "",
            long_name: "",
            desc: "",
            has_arg: HasArg.No,
            occur: Occur.Optional,
        };
    }
}

pub struct OptVal {
    opt_idx: usize,
    has_val: bool,
    val: string,

    static fn empty(): OptVal {
        return OptVal{
            opt_idx: 0,
            has_val: false,
            val: "",
        };
    }
}

fn goptStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn goptFindEq(s: string): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '=') {
            return i;
        }
    }
    return null;
}

fn goptFindByName(defs: []const OptDef, count: usize, name: string): usize? {
    if (name.len == 0) {
        return null;
    }
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (defs[i].short_name.len > 0 && goptStrEq(defs[i].short_name, name)) {
            return i;
        }
        if (defs[i].long_name.len > 0 && goptStrEq(defs[i].long_name, name)) {
            return i;
        }
    }
    return null;
}

fn goptFindShort(defs: []const OptDef, count: usize, ch: string): usize? {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (defs[i].short_name.len > 0 && goptStrEq(defs[i].short_name, ch)) {
            return i;
        }
    }
    return null;
}

fn goptFindLong(defs: []const OptDef, count: usize, name: string): usize? {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (defs[i].long_name.len > 0 && goptStrEq(defs[i].long_name, name)) {
            return i;
        }
    }
    return null;
}

fn goptCountForIdx(vals: []const OptVal, val_count: usize, idx: usize): usize {
    let c: usize = 0;
    let i: usize = 0;
    while (i < val_count) : (i += 1) {
        if (vals[i].opt_idx == idx) {
            c += 1;
        }
    }
    return c;
}

/// Parsed command-line match results.
pub struct Matches {
    ok: bool,
    err_kind: i32,
    err_opt: string,
    defs: [32]OptDef,
    def_count: usize,
    vals: [64]OptVal,
    val_count: usize,
    free: [64]string,
    free_count: usize,

    fn optPresent(name: string): bool {
        return _.optCount(name) > 0;
    }

    fn optCount(name: string): usize {
        if (idx := goptFindByName(_.defs[0.._.def_count], _.def_count, name)) {
            return goptCountForIdx(_.vals[0.._.val_count], _.val_count, idx);
        }
        return 0;
    }

    fn optStr(name: string): string? {
        if (idx := goptFindByName(_.defs[0.._.def_count], _.def_count, name)) {
            let i: usize = 0;
            while (i < _.val_count) : (i += 1) {
                if (_.vals[i].opt_idx == idx && _.vals[i].has_val) {
                    return _.vals[i].val;
                }
            }
        }
        return null;
    }

    fn optStrs(name: string, out: []string): usize {
        let written: usize = 0;
        if (idx := goptFindByName(_.defs[0.._.def_count], _.def_count, name)) {
            let i: usize = 0;
            while (i < _.val_count && written < out.len) : (i += 1) {
                if (_.vals[i].opt_idx == idx && _.vals[i].has_val) {
                    out[written] = _.vals[i].val;
                    written += 1;
                }
            }
        }
        return written;
    }

    fn freeCount(): usize {
        return _.free_count;
    }

    fn freeAt(idx: usize): string? {
        if (idx >= _.free_count) {
            return null;
        }
        return _.free[idx];
    }
}

/// Command-line option schema & parser (up to 32 option definitions).
pub struct Options {
    defs: [32]OptDef,
    def_count: usize,

    static fn new(): Options {
        return Options{
            defs: [OptDef.empty(); 32],
            def_count: 0,
        };
    }

    fn opt(*_, short: string, long: string, desc: string, has_arg: HasArg, occur: Occur) {
        if (_.def_count >= GOPT_MAX_OPTS) {
            return;
        }
        _.defs[_.def_count] = OptDef{
            short_name: short,
            long_name: long,
            desc: desc,
            has_arg: has_arg,
            occur: occur,
        };
        _.def_count += 1;
    }

    fn optflag(*_, short: string, long: string, desc: string) {
        _.opt(short, long, desc, HasArg.No, Occur.Optional);
    }

    fn optflagmulti(*_, short: string, long: string, desc: string) {
        _.opt(short, long, desc, HasArg.No, Occur.Multi);
    }

    fn optopt(*_, short: string, long: string, desc: string) {
        _.opt(short, long, desc, HasArg.Yes, Occur.Optional);
    }

    fn optmulti(*_, short: string, long: string, desc: string) {
        _.opt(short, long, desc, HasArg.Yes, Occur.Multi);
    }

    fn reqopt(*_, short: string, long: string, desc: string) {
        _.opt(short, long, desc, HasArg.Yes, Occur.Req);
    }

    fn parse(args: []const string): Matches {
        let m: Matches = Matches{
            ok: true,
            err_kind: GOPT_OK,
            err_opt: "",
            defs: _.defs,
            def_count: _.def_count,
            vals: [OptVal.empty(); 64],
            val_count: 0,
            free: [""; 64],
            free_count: 0,
        };

        let seen_double_dash: bool = false;
        let i: usize = 0;

        while (i < args.len) : (i += 1) {
            const arg: string = args[i];

            if (seen_double_dash) {
                if (m.free_count < GOPT_MAX_FREE) {
                    m.free[m.free_count] = arg;
                    m.free_count += 1;
                }
                continue;
            }

            if (goptStrEq(arg, "--")) {
                seen_double_dash = true;
                continue;
            }

            // Long option: --name or --name=value
            if (arg.len > 2 && arg[0] == '-' && arg[1] == '-') {
                const body: string = arg[2..arg.len];
                let name: string = body;
                let has_inline: bool = false;
                let inline_val: string = "";

                if (eq_pos := goptFindEq(body)) {
                    name = body[0..eq_pos];
                    has_inline = true;
                    inline_val = body[eq_pos + 1..body.len];
                }

                const opt_idx_opt: usize? = goptFindLong(m.defs[0..m.def_count], m.def_count, name);
                if (opt_idx_opt == null) {
                    m.ok = false;
                    m.err_kind = GOPT_ERR_UNRECOGNIZED_OPTION;
                    m.err_opt = name;
                    return m;
                }
                const idx: usize = opt_idx_opt.?;
                const def: OptDef = m.defs[idx];

                if (def.occur != Occur.Multi && goptCountForIdx(m.vals[0..m.val_count], m.val_count, idx) > 0) {
                    m.ok = false;
                    m.err_kind = GOPT_ERR_OPTION_DUPLICATED;
                    m.err_opt = name;
                    return m;
                }

                if (def.has_arg == HasArg.No) {
                    if (has_inline) {
                        m.ok = false;
                        m.err_kind = GOPT_ERR_UNEXPECTED_ARGUMENT;
                        m.err_opt = name;
                        return m;
                    }
                    if (m.val_count < GOPT_MAX_VALUES) {
                        m.vals[m.val_count] = OptVal{ opt_idx: idx, has_val: false, val: "" };
                        m.val_count += 1;
                    }
                } else if (def.has_arg == HasArg.Yes) {
                    let val_str: string = "";
                    if (has_inline) {
                        val_str = inline_val;
                    } else if (i + 1 < args.len) {
                        i += 1;
                        val_str = args[i];
                    } else {
                        m.ok = false;
                        m.err_kind = GOPT_ERR_ARGUMENT_MISSING;
                        m.err_opt = name;
                        return m;
                    }
                    if (m.val_count < GOPT_MAX_VALUES) {
                        m.vals[m.val_count] = OptVal{ opt_idx: idx, has_val: true, val: val_str };
                        m.val_count += 1;
                    }
                } else {
                    // HasArg.Maybe
                    if (m.val_count < GOPT_MAX_VALUES) {
                        m.vals[m.val_count] = OptVal{ opt_idx: idx, has_val: has_inline, val: inline_val };
                        m.val_count += 1;
                    }
                }
                continue;
            }

            // Short option(s): -a, -abc, -o val, -oval
            if (arg.len >= 2 && arg[0] == '-') {
                let j: usize = 1;
                while (j < arg.len) : (j += 1) {
                    const ch: string = arg[j..j + 1];
                    const opt_idx_opt: usize? = goptFindShort(m.defs[0..m.def_count], m.def_count, ch);
                    if (opt_idx_opt == null) {
                        m.ok = false;
                        m.err_kind = GOPT_ERR_UNRECOGNIZED_OPTION;
                        m.err_opt = ch;
                        return m;
                    }
                    const idx: usize = opt_idx_opt.?;
                    const def: OptDef = m.defs[idx];

                    if (def.occur != Occur.Multi && goptCountForIdx(m.vals[0..m.val_count], m.val_count, idx) > 0) {
                        m.ok = false;
                        m.err_kind = GOPT_ERR_OPTION_DUPLICATED;
                        m.err_opt = ch;
                        return m;
                    }

                    if (def.has_arg == HasArg.No) {
                        if (m.val_count < GOPT_MAX_VALUES) {
                            m.vals[m.val_count] = OptVal{ opt_idx: idx, has_val: false, val: "" };
                            m.val_count += 1;
                        }
                    } else if (def.has_arg == HasArg.Yes) {
                        let val_str: string = "";
                        if (j + 1 < arg.len) {
                            val_str = arg[j + 1..arg.len];
                        } else if (i + 1 < args.len) {
                            i += 1;
                            val_str = args[i];
                        } else {
                            m.ok = false;
                            m.err_kind = GOPT_ERR_ARGUMENT_MISSING;
                            m.err_opt = ch;
                            return m;
                        }
                        if (m.val_count < GOPT_MAX_VALUES) {
                            m.vals[m.val_count] = OptVal{ opt_idx: idx, has_val: true, val: val_str };
                            m.val_count += 1;
                        }
                        break;
                    } else {
                        // HasArg.Maybe
                        if (j + 1 < arg.len) {
                            if (m.val_count < GOPT_MAX_VALUES) {
                                m.vals[m.val_count] = OptVal{ opt_idx: idx, has_val: true, val: arg[j + 1..arg.len] };
                                m.val_count += 1;
                            }
                            break;
                        } else {
                            if (m.val_count < GOPT_MAX_VALUES) {
                                m.vals[m.val_count] = OptVal{ opt_idx: idx, has_val: false, val: "" };
                                m.val_count += 1;
                            }
                        }
                    }
                }
                continue;
            }

            // Positional free argument (including bare "-")
            if (m.free_count < GOPT_MAX_FREE) {
                m.free[m.free_count] = arg;
                m.free_count += 1;
            }
        }

        // Verify all required options were provided
        let k: usize = 0;
        while (k < m.def_count) : (k += 1) {
            if (m.defs[k].occur == Occur.Req) {
                if (goptCountForIdx(m.vals[0..m.val_count], m.val_count, k) == 0) {
                    m.ok = false;
                    m.err_kind = GOPT_ERR_OPTION_MISSING;
                    m.err_opt = if (m.defs[k].long_name.len > 0) m.defs[k].long_name else m.defs[k].short_name;
                    return m;
                }
            }
        }

        return m;
    }
}
