// @cap/optionator — Zero-heap CLI option spec & aligned --help generator (clean-room CP)

pub struct OptionSpec {
    long_name: string,
    short_alias: string,
    value_type: string,
    description: string,
    default_val: string,
    required: bool,
}

fn optnStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn optnWriteStr(out: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn optnWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn optnWriteSpaces(out: []u8, pos: usize, count: usize): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < count && p < out.len) : (i += 1) {
        out[p] = ' ';
        p += 1;
    }
    return p;
}

fn optnLeftColLen(spec: OptionSpec): usize {
    let n: usize = 2; // leading "  "
    let has_short = spec.short_alias.len > 0;
    let has_long = spec.long_name.len > 0;
    if (has_short) {
        n += 1 + spec.short_alias.len; // "-p"
    }
    if (has_short && has_long) {
        n += 2; // ", "
    }
    if (has_long) {
        n += 2 + spec.long_name.len; // "--port"
    }
    if (spec.value_type.len > 0) {
        n += 1 + spec.value_type.len; // " Int"
    }
    return n;
}

pub fn formatOptionRow(spec: OptionSpec, col_width: usize, out: []u8): usize {
    let pos: usize = 0;
    pos = optnWriteStr(out, pos, "  ");
    let has_short = spec.short_alias.len > 0;
    let has_long = spec.long_name.len > 0;
    if (has_short) {
        pos = optnWriteByte(out, pos, '-');
        pos = optnWriteStr(out, pos, spec.short_alias);
    }
    if (has_short && has_long) {
        pos = optnWriteStr(out, pos, ", ");
    }
    if (has_long) {
        pos = optnWriteStr(out, pos, "--");
        pos = optnWriteStr(out, pos, spec.long_name);
    }
    if (spec.value_type.len > 0) {
        pos = optnWriteByte(out, pos, ' ');
        pos = optnWriteStr(out, pos, spec.value_type);
    }

    let has_right = spec.description.len > 0 || spec.default_val.len > 0 || spec.required;
    if (!has_right) {
        return pos;
    }

    let left_len = optnLeftColLen(spec);
    let pad: usize = 2;
    if (col_width > left_len && col_width - left_len > 2) {
        pad = col_width - left_len;
    }
    pos = optnWriteSpaces(out, pos, pad);

    let wrote_any = false;
    if (spec.description.len > 0) {
        pos = optnWriteStr(out, pos, spec.description);
        wrote_any = true;
    }
    if (spec.required) {
        if (wrote_any) {
            pos = optnWriteByte(out, pos, ' ');
        }
        pos = optnWriteStr(out, pos, "[required]");
        wrote_any = true;
    }
    if (spec.default_val.len > 0) {
        if (wrote_any) {
            pos = optnWriteByte(out, pos, ' ');
        }
        pos = optnWriteStr(out, pos, "(default: ");
        pos = optnWriteStr(out, pos, spec.default_val);
        pos = optnWriteByte(out, pos, ')');
    }

    return pos;
}

pub fn generateHelp(
    program: string,
    summary: string,
    specs: []const OptionSpec,
    out: []u8,
): usize {
    let pos: usize = 0;
    if (program.len > 0) {
        pos = optnWriteStr(out, pos, "Usage: ");
        pos = optnWriteStr(out, pos, program);
        pos = optnWriteStr(out, pos, " [options]\n");
    }
    if (summary.len > 0) {
        pos = optnWriteStr(out, pos, summary);
        pos = optnWriteByte(out, pos, '\n');
    }
    if (specs.len > 0) {
        if (pos > 0) {
            pos = optnWriteByte(out, pos, '\n');
        }
        pos = optnWriteStr(out, pos, "Options:\n");

        let max_left: usize = 0;
        let i: usize = 0;
        while (i < specs.len) : (i += 1) {
            let l = optnLeftColLen(specs[i]);
            if (l > max_left) {
                max_left = l;
            }
        }
        let col_width: usize = max_left + 2;

        let j: usize = 0;
        while (j < specs.len) : (j += 1) {
            let n = formatOptionRow(specs[j], col_width, out[pos..out.len]);
            pos += n;
            pos = optnWriteByte(out, pos, '\n');
        }
    }
    return pos;
}

pub fn findOptionSpec(specs: []const OptionSpec, flag: string): OptionSpec? {
    if (flag.len >= 3 && flag[0] == '-' && flag[1] == '-') {
        let name: string = flag[2..flag.len];
        let i: usize = 0;
        while (i < specs.len) : (i += 1) {
            if (specs[i].long_name.len > 0 && optnStrEq(specs[i].long_name, name)) {
                return specs[i];
            }
        }
        return null;
    }
    if (flag.len >= 2 && flag[0] == '-' && flag[1] != '-') {
        let alias: string = flag[1..flag.len];
        let i: usize = 0;
        while (i < specs.len) : (i += 1) {
            if (specs[i].short_alias.len > 0 && optnStrEq(specs[i].short_alias, alias)) {
                return specs[i];
            }
        }
        return null;
    }
    return null;
}
