// @cap/optionator — Zero-heap CLI option spec & aligned --help generator (clean-room CP)

import { strEq } from "cap:ascii";
import { BufWriter } from "cap:fmt";

pub struct OptionSpec {
    long_name: string,
    short_alias: string,
    value_type: string,
    description: string,
    default_val: string,
    required: bool,
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
    let w = BufWriter.new(out);
    w.writeStr("  ");
    let has_short = spec.short_alias.len > 0;
    let has_long = spec.long_name.len > 0;
    if (has_short) {
        w.writeByte('-');
        w.writeStr(spec.short_alias);
    }
    if (has_short && has_long) {
        w.writeStr(", ");
    }
    if (has_long) {
        w.writeStr("--");
        w.writeStr(spec.long_name);
    }
    if (spec.value_type.len > 0) {
        w.writeByte(' ');
        w.writeStr(spec.value_type);
    }

    let has_right = spec.description.len > 0 || spec.default_val.len > 0 || spec.required;
    if (!has_right) {
        return w.written();
    }

    let left_len = optnLeftColLen(spec);
    let pad: usize = 2;
    if (col_width > left_len && col_width - left_len > 2) {
        pad = col_width - left_len;
    }
    w.writeRepeat(' ', pad);

    let wrote_any = false;
    if (spec.description.len > 0) {
        w.writeStr(spec.description);
        wrote_any = true;
    }
    if (spec.required) {
        if (wrote_any) {
            w.writeByte(' ');
        }
        w.writeStr("[required]");
        wrote_any = true;
    }
    if (spec.default_val.len > 0) {
        if (wrote_any) {
            w.writeByte(' ');
        }
        w.writeStr("(default: ");
        w.writeStr(spec.default_val);
        w.writeByte(')');
    }

    return w.written();
}

pub fn generateHelp(
    program: string,
    summary: string,
    specs: []const OptionSpec,
    out: []u8,
): usize {
    let w = BufWriter.new(out);
    if (program.len > 0) {
        w.writeStr("Usage: ");
        w.writeStr(program);
        w.writeStr(" [options]\n");
    }
    if (summary.len > 0) {
        w.writeStr(summary);
        w.writeByte('\n');
    }
    if (specs.len > 0) {
        if (w.written() > 0) {
            w.writeByte('\n');
        }
        w.writeStr("Options:\n");

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
            let pos = w.written();
            let n = formatOptionRow(specs[j], col_width, out[pos..out.len]);
            w.pos = pos + n;
            w.writeByte('\n');
        }
    }
    return w.written();
}

pub fn findOptionSpec(specs: []const OptionSpec, flag: string): OptionSpec? {
    if (flag.len >= 3 && flag[0] == '-' && flag[1] == '-') {
        let name: string = flag[2..flag.len];
        let i: usize = 0;
        while (i < specs.len) : (i += 1) {
            if (specs[i].long_name.len > 0 && strEq(specs[i].long_name, name)) {
                return specs[i];
            }
        }
        return null;
    }
    if (flag.len >= 2 && flag[0] == '-' && flag[1] != '-') {
        let alias: string = flag[1..flag.len];
        let i: usize = 0;
        while (i < specs.len) : (i += 1) {
            if (specs[i].short_alias.len > 0 && strEq(specs[i].short_alias, alias)) {
                return specs[i];
            }
        }
        return null;
    }
    return null;
}
